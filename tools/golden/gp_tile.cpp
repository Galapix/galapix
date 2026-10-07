// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Golden-path tile encode/decode matrix (vips only — no thumtoo Client/Store).
// Splits images into 256² cells at scale 0 and measures encode/decode
// (jpeg|webp|avif|jxl) at several quality settings, plus PSNR of the decoded
// cells against the source. With several codecs it judges which wins at equal
// image quality. See --help.
//
// Comparison mode never compares codecs at the same Q number (JPEG Q80 and
// AVIF Q80 are unrelated). It sweeps qualities, takes the PSNR of the
// reference setting (default jpeg:80 = thumtoo's kDefaultTileCodec /
// kDefaultTileQuality) as the target, represents each codec by its smallest
// output that reaches the target, and judges bytes / encode / decode there.
//
// Codec availability is probed at runtime (libvips may lack an encoder,
// e.g. heifsave without an AV1 encoder plugin). An unavailable codec is
// reported as unsupported, never as a zero-byte / zero-ms result.

#include <vips/vips.h>

#include "gp_cli.hpp"
#include "gp_common.hpp"
#include "gp_json.hpp"
#include "gp_output.hpp"
#include "gp_verdict.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <optional>
#include <sstream>
#include <utility>
#include <string>
#include <vector>

namespace {

namespace fs = std::filesystem;

// PSNR reported for bit-exact output (MSE == 0).
constexpr double kLosslessPsnr = 100.0;

void ensure_vips() {
  static bool once = false;
  if (!once) {
    if (VIPS_INIT("gp_tile")) {
      std::cerr << "VIPS_INIT failed\n";
      std::exit(1);
    }
    once = true;
  }
}

/// First line of the vips error buffer (then clears it).
std::string take_vips_error() {
  std::string s = vips_error_buffer();
  vips_error_clear();
  if (auto nl = s.find('\n'); nl != std::string::npos) s.resize(nl);
  return s.empty() ? "unknown libvips error" : s;
}

// --- codecs ---------------------------------------------------------------------

enum class Codec { Jpeg, Webp, Avif, Jxl };

constexpr Codec kAllCodecs[] = {Codec::Jpeg, Codec::Webp, Codec::Avif, Codec::Jxl};

const char* codec_name(Codec c) {
  switch (c) {
    case Codec::Jpeg: return "jpeg";
    case Codec::Webp: return "webp";
    case Codec::Avif: return "avif";
    case Codec::Jxl: return "jxl";
  }
  return "?";
}

std::optional<Codec> parse_codec(const std::string& s) {
  for (Codec c : kAllCodecs) {
    if (s == codec_name(c)) return c;
  }
  return std::nullopt;
}

/// Inclusive libvips "effort" range per codec; {0, -1} = no effort knob.
std::pair<int, int> effort_range(Codec c) {
  switch (c) {
    case Codec::Jpeg: return {0, -1};
    case Codec::Webp: return {0, 6};
    case Codec::Avif: return {0, 9};
    case Codec::Jxl: return {1, 9};
  }
  return {0, -1};
}

/// A codec plus encoder settings — the unit that gets measured and ranked.
struct Variant {
  Codec codec = Codec::Jpeg;
  int effort = -1;  // -1: libvips default

  std::string name() const {
    std::string n = codec_name(codec);
    if (effort >= 0) n += "@e" + std::to_string(effort);
    return n;
  }
  bool operator==(const Variant& o) const {
    return codec == o.codec && effort == o.effort;
  }
};

/// Parse "codec" or "codec@eN"; error text in `err` on failure.
std::optional<Variant> parse_variant(const std::string& s, std::string& err) {
  const auto at = s.find('@');
  const auto codec = parse_codec(s.substr(0, at));
  if (!codec) {
    err = "unknown codec '" + s.substr(0, at) + "' (want jpeg|webp|avif|jxl)";
    return std::nullopt;
  }
  Variant v{*codec, -1};
  if (at == std::string::npos) return v;
  const std::string opt = s.substr(at + 1);
  const auto [lo, hi] = effort_range(*codec);
  if (hi < lo) {
    err = std::string(codec_name(*codec)) + " has no effort setting";
    return std::nullopt;
  }
  char* end = nullptr;
  const long e = opt.size() > 1 && opt[0] == 'e' ? std::strtol(opt.c_str() + 1, &end, 10) : -1;
  if (!end || *end != '\0' || e < lo || e > hi) {
    err = "bad option '" + opt + "' for " + codec_name(*codec) + " (want e" +
          std::to_string(lo) + "..e" + std::to_string(hi) + ")";
    return std::nullopt;
  }
  v.effort = static_cast<int>(e);
  return v;
}

/// Encode at Q (and the variant's effort, else the libvips default).
/// false on failure.
bool encode(const Variant& v, VipsImage* in, int q, std::vector<unsigned char>& out) {
  void* buf = nullptr;
  size_t len = 0;
  int rc = -1;
  // The effort pair is appended only when set, so defaults stay libvips'.
  const bool fx = v.effort >= 0;
  switch (v.codec) {
    case Codec::Jpeg:
      rc = vips_jpegsave_buffer(in, &buf, &len, "Q", q, "strip", TRUE, nullptr);
      break;
    case Codec::Webp:
      rc = fx ? vips_webpsave_buffer(in, &buf, &len, "Q", q, "effort", v.effort, nullptr)
              : vips_webpsave_buffer(in, &buf, &len, "Q", q, nullptr);
      break;
    case Codec::Avif:
      // AVIF via the HEIF saver. Needs libvips + libheif with an AV1 encoder.
      rc = fx ? vips_heifsave_buffer(in, &buf, &len, "Q", q, "compression",
                                     VIPS_FOREIGN_HEIF_COMPRESSION_AV1, "effort",
                                     v.effort, nullptr)
              : vips_heifsave_buffer(in, &buf, &len, "Q", q, "compression",
                                     VIPS_FOREIGN_HEIF_COMPRESSION_AV1, nullptr);
      break;
    case Codec::Jxl:
      rc = fx ? vips_jxlsave_buffer(in, &buf, &len, "Q", q, "effort", v.effort, nullptr)
              : vips_jxlsave_buffer(in, &buf, &len, "Q", q, nullptr);
      break;
  }
  if (rc != 0 || !buf || len == 0) {
    if (buf) g_free(buf);
    return false;
  }
  out.assign(static_cast<unsigned char*>(buf), static_cast<unsigned char*>(buf) + len);
  g_free(buf);
  return true;
}

/// Decode a blob; nullptr on failure. Caller unrefs.
VipsImage* decode(Codec codec, const std::vector<unsigned char>& blob) {
  void* data = const_cast<unsigned char*>(blob.data());
  VipsImage* img = nullptr;
  int rc = -1;
  switch (codec) {
    case Codec::Jpeg: rc = vips_jpegload_buffer(data, blob.size(), &img, nullptr); break;
    case Codec::Webp: rc = vips_webpload_buffer(data, blob.size(), &img, nullptr); break;
    case Codec::Avif: rc = vips_heifload_buffer(data, blob.size(), &img, nullptr); break;
    case Codec::Jxl: rc = vips_jxlload_buffer(data, blob.size(), &img, nullptr); break;
  }
  if (rc != 0) {
    if (img) g_object_unref(img);
    return nullptr;
  }
  return img;
}

/// Decode fully into pixels (the cost a tile consumer pays). false on failure.
bool decode_pixels(Codec codec, const std::vector<unsigned char>& blob,
                   std::vector<unsigned char>* pixels, int* w, int* h, int* bands) {
  VipsImage* img = decode(codec, blob);
  if (!img) return false;
  size_t len = 0;
  void* buf = vips_image_write_to_memory(img, &len);
  const bool ok = buf != nullptr;
  if (ok && pixels) {
    if (vips_image_get_format(img) != VIPS_FORMAT_UCHAR) {
      // Tiles are 8-bit; a wider decode would not be comparable.
      g_free(buf);
      g_object_unref(img);
      return false;
    }
    pixels->assign(static_cast<unsigned char*>(buf), static_cast<unsigned char*>(buf) + len);
    *w = vips_image_get_width(img);
    *h = vips_image_get_height(img);
    *bands = vips_image_get_bands(img);
  }
  if (buf) g_free(buf);
  g_object_unref(img);
  return ok;
}

// --- source ---------------------------------------------------------------------

struct Cell {
  int x = 0, y = 0, w = 0, h = 0;
};

std::vector<Cell> grid_cells(int img_w, int img_h, int tile) {
  std::vector<Cell> cells;
  const int nx = (img_w + tile - 1) / tile;
  const int ny = (img_h + tile - 1) / tile;
  cells.reserve(static_cast<std::size_t>(nx * ny));
  for (int ty = 0; ty < ny; ++ty) {
    for (int tx = 0; tx < nx; ++tx) {
      Cell c;
      c.x = tx * tile;
      c.y = ty * tile;
      c.w = std::min(tile, img_w - c.x);
      c.h = std::min(tile, img_h - c.y);
      cells.push_back(c);
    }
  }
  return cells;
}

/// Decoded source image held in memory, as 8-bit sRGB or B_W without alpha
/// (what thumtoo tiles store), plus each cell's crop and reference pixels.
struct Source {
  VipsImage* mem = nullptr;
  int width = 0, height = 0, bands = 0;
  std::string loader;  // e.g. "jpegload", "pngload"
  bool lossy = false;  // source itself was lossy-coded
  std::vector<Cell> cells;
  std::vector<VipsImage*> crops;
  std::vector<std::vector<unsigned char>> ref_pixels;

  Source() = default;
  Source(const Source&) = delete;
  Source& operator=(const Source&) = delete;
  ~Source() {
    for (auto* c : crops) g_object_unref(c);
    if (mem) g_object_unref(mem);
  }
};

/// Replace *img with the result of op (unref old) — false on failure.
template <typename Op>
bool apply(VipsImage** img, Op op) {
  VipsImage* out = nullptr;
  if (op(*img, &out) != 0 || !out) return false;
  g_object_unref(*img);
  *img = out;
  return true;
}

bool load_source(const fs::path& file, int tile, Source& src, std::string& err) {
  VipsImage* img = vips_image_new_from_file(file.string().c_str(), nullptr);
  if (!img) {
    err = "open failed: " + take_vips_error();
    return false;
  }
  const char* loader = nullptr;
  if (vips_image_get_typeof(img, "vips-loader") &&
      vips_image_get_string(img, "vips-loader", &loader) == 0 && loader) {
    src.loader = loader;
  }
  for (const char* lossy : {"jpegload", "webpload", "heifload", "jxlload"}) {
    if (src.loader.rfind(lossy, 0) == 0) src.lossy = true;  // also *_source etc.
  }
  bool ok = true;
  if (vips_image_hasalpha(img)) {
    ok = apply(&img, [](VipsImage* in, VipsImage** out) {
      VipsArrayDouble* white = vips_array_double_newv(1, 255.0);
      const int rc = vips_flatten(in, out, "background", white, nullptr);
      vips_area_unref(VIPS_AREA(white));
      return rc;
    });
  }
  if (ok && vips_image_get_format(img) != VIPS_FORMAT_UCHAR) {
    // colourspace() rescales 16-bit / float data; a plain cast would clip.
    const VipsInterpretation target = vips_image_get_bands(img) >= 3
                                          ? VIPS_INTERPRETATION_sRGB
                                          : VIPS_INTERPRETATION_B_W;
    ok = apply(&img, [target](VipsImage* in, VipsImage** out) {
      return vips_colourspace(in, out, target, nullptr);
    });
  }
  if (ok) {
    // Decode once, fully, so encode timings never include source decode.
    VipsImage* mem = vips_image_copy_memory(img);
    ok = mem != nullptr;
    if (ok) {
      g_object_unref(img);
      img = mem;
    }
  }
  if (!ok) {
    g_object_unref(img);
    err = "prepare failed: " + take_vips_error();
    return false;
  }

  src.mem = img;
  src.width = vips_image_get_width(img);
  src.height = vips_image_get_height(img);
  src.bands = vips_image_get_bands(img);
  src.cells = grid_cells(src.width, src.height, tile);
  for (const Cell& c : src.cells) {
    VipsImage* crop = nullptr;
    if (vips_crop(src.mem, &crop, c.x, c.y, c.w, c.h, nullptr) != 0 || !crop) {
      err = "crop failed: " + take_vips_error();
      return false;
    }
    src.crops.push_back(crop);
    size_t len = 0;
    void* buf = vips_image_write_to_memory(crop, &len);
    if (!buf) {
      err = "crop read failed: " + take_vips_error();
      return false;
    }
    src.ref_pixels.emplace_back(static_cast<unsigned char*>(buf),
                                static_cast<unsigned char*>(buf) + len);
    g_free(buf);
  }
  return true;
}

// --- measurement ----------------------------------------------------------------

enum class Status { Ok, Unsupported, Failed };

const char* status_name(Status s) {
  switch (s) {
    case Status::Ok: return "ok";
    case Status::Unsupported: return "unsupported";
    case Status::Failed: return "failed";
  }
  return "failed";
}

struct Row {
  int quality = 0;
  gp::Timing enc, dec;
  std::uint64_t bytes_total = 0;
  double bytes_per_cell_mean = 0;
  double psnr_db = 0;
};

struct CodecResult {
  Variant variant;
  Status status = Status::Failed;
  std::string reason;
  std::vector<Row> rows;
};

/// Encode + decode a tiny synthetic image: does this libvips build have a
/// working encoder *and* decoder for `codec`? Returns "" or the reason.
std::string probe_codec(const Variant& variant) {
  VipsImage* black = nullptr;
  VipsImage* rgb = nullptr;
  std::string why;
  if (vips_black(&black, 16, 16, "bands", 3, nullptr) != 0 ||
      vips_cast_uchar(black, &rgb, nullptr) != 0) {
    why = "probe image: " + take_vips_error();
  } else {
    std::vector<unsigned char> blob;
    if (!encode(variant, rgb, 80, blob)) {
      why = "encoder unavailable: " + take_vips_error();
    } else if (!decode_pixels(variant.codec, blob, nullptr, nullptr, nullptr, nullptr)) {
      why = "decoder unavailable: " + take_vips_error();
    }
  }
  if (rgb) g_object_unref(rgb);
  if (black) g_object_unref(black);
  return why;
}

/// Sum of squared errors over the bands both images share. Decoders may add
/// bands (gray → RGB); fewer bands or a size change is a failure.
std::optional<double> squared_error(const std::vector<unsigned char>& ref, int ref_bands,
                                    const std::vector<unsigned char>& got, int got_bands,
                                    std::uint64_t pixels) {
  if (got_bands < ref_bands ||
      got.size() != pixels * static_cast<std::uint64_t>(got_bands) ||
      ref.size() != pixels * static_cast<std::uint64_t>(ref_bands)) {
    return std::nullopt;
  }
  double sse = 0;
  for (std::uint64_t p = 0; p < pixels; ++p) {
    for (int b = 0; b < ref_bands; ++b) {
      const double d = static_cast<double>(ref[p * ref_bands + b]) -
                       static_cast<double>(got[p * got_bands + b]);
      sse += d * d;
    }
  }
  return sse;
}

/// Full-grid bytes + PSNR for one setting; timings only when `timed`.
std::optional<Row> measure_quality(const Variant& variant, int q, const Source& src, int time_n,
                                   int repeats, bool timed, std::string& err) {
  const std::size_t n = src.cells.size();
  std::vector<std::vector<unsigned char>> blobs(n);
  Row row;
  row.quality = q;

  // Full grid once: bytes and PSNR over every cell.
  double sse = 0;
  std::uint64_t samples = 0;
  for (std::size_t i = 0; i < n; ++i) {
    if (!encode(variant, src.crops[i], q, blobs[i])) {
      err = "encode failed at cell " + std::to_string(i) + ": " + take_vips_error();
      return std::nullopt;
    }
    row.bytes_total += blobs[i].size();
    std::vector<unsigned char> px;
    int w = 0, h = 0, bands = 0;
    if (!decode_pixels(variant.codec, blobs[i], &px, &w, &h, &bands)) {
      err = "decode failed at cell " + std::to_string(i) + ": " + take_vips_error();
      return std::nullopt;
    }
    const Cell& c = src.cells[i];
    if (w != c.w || h != c.h) {
      err = "decoded cell " + std::to_string(i) + " has wrong size";
      return std::nullopt;
    }
    const std::uint64_t pixels = static_cast<std::uint64_t>(w) * h;
    auto e = squared_error(src.ref_pixels[i], src.bands, px, bands, pixels);
    if (!e) {
      err = "decoded cell " + std::to_string(i) + " has incompatible bands";
      return std::nullopt;
    }
    sse += *e;
    samples += pixels * static_cast<std::uint64_t>(src.bands);
  }
  const double mse = samples ? sse / static_cast<double>(samples) : 0.0;
  row.psnr_db = mse > 0 ? 10.0 * std::log10(255.0 * 255.0 / mse) : kLosslessPsnr;
  row.bytes_per_cell_mean = n ? static_cast<double>(row.bytes_total) / n : 0.0;
  if (!timed) return row;

  // Timed subset (all outputs were verified above).
  std::vector<unsigned char> scratch;
  row.enc = gp::time_median(repeats, [&] {
    for (int i = 0; i < time_n; ++i) (void)encode(variant, src.crops[i], q, scratch);
  });
  row.dec = gp::time_median(repeats, [&] {
    for (int i = 0; i < time_n; ++i) {
      (void)decode_pixels(variant.codec, blobs[i], nullptr, nullptr, nullptr, nullptr);
    }
  });
  return row;
}

CodecResult measure_codec(const Variant& variant, const std::vector<int>& qualities,
                          const Source& src, int time_n, int repeats) {
  CodecResult r;
  r.variant = variant;
  if (auto why = probe_codec(variant); !why.empty()) {
    r.status = Status::Unsupported;
    r.reason = why;
    return r;
  }
  for (int q : qualities) {
    std::string err;
    auto row = measure_quality(variant, q, src, time_n, repeats, /*timed=*/true, err);
    if (!row) {
      r.status = Status::Failed;
      r.reason = "q=" + std::to_string(q) + ": " + err;
      r.rows.clear();
      return r;
    }
    r.rows.push_back(*row);
  }
  r.status = Status::Ok;
  return r;
}

// --- comparison -----------------------------------------------------------------

struct Reference {
  Variant variant;   // jpeg
  int quality = 80;  // thumtoo kDefaultTileCodec / kDefaultTileQuality

  std::string name() const { return variant.name() + ":" + std::to_string(quality); }
};

/// "VARIANT:Q", e.g. jpeg:80 or jxl@e3:75.
std::optional<Reference> parse_reference(const std::string& s, std::string& err) {
  const auto colon = s.rfind(':');
  if (colon == std::string::npos) {
    err = "missing :Q";
    return std::nullopt;
  }
  auto variant = parse_variant(s.substr(0, colon), err);
  if (!variant) return std::nullopt;
  char* end = nullptr;
  const long q = std::strtol(s.c_str() + colon + 1, &end, 10);
  if (!end || *end != '\0' || q < 1 || q > 100) {
    err = "quality must be 1..100";
    return std::nullopt;
  }
  return Reference{*variant, static_cast<int>(q)};
}

/// A codec's representative row at the reference quality level.
struct Match {
  Variant variant;
  const Row* row = nullptr;   // nullptr: no setting reached the target
  const Row* best = nullptr;  // highest-PSNR row (for the "below target" note)
};

/// Smallest output whose PSNR reaches `target_db` (ties: faster decode).
Match match_quality(const CodecResult& r, double target_db) {
  Match m;
  m.variant = r.variant;
  for (const Row& row : r.rows) {
    if (!m.best || row.psnr_db > m.best->psnr_db) m.best = &row;
    if (row.psnr_db + 1e-9 < target_db) continue;
    if (!m.row || row.bytes_total < m.row->bytes_total ||
        (row.bytes_total == m.row->bytes_total && row.dec.median < m.row->dec.median)) {
      m.row = &row;
    }
  }
  return m;
}

/// Narrow a codec's match to the lowest Q that reaches `target_db`.
///
/// A coarse sweep overshoots: if q70 is just below the target the match is
/// q80, charging the codec for quality it was not asked for. Bisect Q between
/// the bracketing sweep points with untimed bytes+PSNR evaluations (assumes
/// PSNR is non-decreasing in Q, which holds closely for these encoders), then
/// time only the final setting and add it to the rows.
void refine_match(CodecResult& r, double target_db, const Source& src, int time_n,
                  int repeats) {
  if (r.status != Status::Ok) return;
  int hi = 0;  // lowest sweep Q reaching the target
  for (const Row& row : r.rows) {
    if (row.psnr_db + 1e-9 >= target_db && (hi == 0 || row.quality < hi)) hi = row.quality;
  }
  if (hi == 0) return;  // nothing reaches it; leave as "below target"
  int lo = 0;  // highest sweep Q below hi that misses (0 = Q1 never measured)
  for (const Row& row : r.rows) {
    if (row.quality < hi && row.psnr_db + 1e-9 < target_db) lo = std::max(lo, row.quality);
  }
  while (hi - lo > 1) {
    const int mid = lo + (hi - lo) / 2;
    std::string err;
    auto row = measure_quality(r.variant, mid, src, time_n, repeats, /*timed=*/false, err);
    if (!row) return;  // keep the sweep result rather than guess
    if (row->psnr_db + 1e-9 >= target_db) hi = mid;
    else lo = mid;
  }
  for (const Row& row : r.rows) {
    if (row.quality == hi) return;  // already measured with timings
  }
  std::string err;
  auto row = measure_quality(r.variant, hi, src, time_n, repeats, /*timed=*/true, err);
  if (!row) return;
  r.rows.push_back(*row);
  std::sort(r.rows.begin(), r.rows.end(),
            [](const Row& a, const Row& b) { return a.quality < b.quality; });
}

const std::vector<gp::MetricSpec>& matched_metrics() {
  static const std::vector<gp::MetricSpec> specs = {
      {"bytes_total", gp::Better::Lower, "size", "size"},
      {"encode_ms", gp::Better::Lower, "encode time", "encode"},
      {"decode_ms", gp::Better::Lower, "decode time", "decode"},
  };
  return specs;
}

/// Everything the renderers need to know about one comparison.
struct CompareOutcome {
  const Row* ref_row = nullptr;  // the reference setting's measured row
  double target = 0;             // its PSNR (dB)
  std::vector<Match> matches;    // point into the CodecResult rows
  gp::Verdict verdict;
  /// A verdict could be reached (reference measured, >= 1 codec matched).
  bool ok() const { return ref_row && !verdict.empty(); }
};

CompareOutcome evaluate_compare(const std::vector<CodecResult>& results,
                                const Reference& ref, double tie_pct) {
  CompareOutcome o;
  for (const auto& r : results) {
    if (!(r.variant == ref.variant) || r.status != Status::Ok) continue;
    for (const Row& row : r.rows) {
      if (row.quality == ref.quality) o.ref_row = &row;
    }
  }
  if (!o.ref_row) return o;
  o.target = o.ref_row->psnr_db;

  std::vector<gp::Candidate> candidates;
  for (const auto& r : results) {
    if (r.status != Status::Ok) continue;
    Match m = match_quality(r, o.target);
    if (m.row) {
      const double bytes = static_cast<double>(m.row->bytes_total);
      candidates.push_back({r.variant.name(),
                            {bytes, m.row->enc.median, m.row->dec.median},
                            {{bytes, bytes},  // deterministic
                             {m.row->enc.min, m.row->enc.max},
                             {m.row->dec.min, m.row->dec.max}}});
    }
    o.matches.push_back(m);
  }
  o.verdict = gp::judge(matched_metrics(), candidates, tie_pct);
  return o;
}

// --- one image ------------------------------------------------------------------

struct Config {
  int tile = 256;
  int repeats = 3;
  int max_time_cells = 16;  // cells timed per image; bytes/PSNR always cover the grid
  std::vector<int> qualities;
  std::vector<Variant> variants;
  bool compare = false;
  Reference reference;
  double tie_pct = 5.0;
};

struct FileRun {
  fs::path path;
  std::string name;        // display name
  std::string load_error;  // non-empty: the image could not be prepared
  int width = 0, height = 0;
  std::size_t cells = 0;
  int time_n = 0;
  std::string loader;
  bool lossy = false;
  std::vector<CodecResult> results;
  CompareOutcome outcome;  // comparison only; points into `results`
  // Not copyable/movable: outcome.matches point into results.
  FileRun() = default;
  FileRun(const FileRun&) = delete;
  FileRun& operator=(const FileRun&) = delete;

  bool loaded() const { return load_error.empty(); }
  bool ok(bool compare) const {
    if (!loaded()) return false;
    return compare ? outcome.ok() : (!results.empty() && results.front().status == Status::Ok);
  }
};

void measure_file(FileRun& run, const Config& cfg) {
  gp::StderrSilencer quiet;  // libvips warnings repeat on every call
  ensure_vips();
  Source src;
  if (!load_source(run.path, cfg.tile, src, run.load_error)) return;
  run.width = src.width;
  run.height = src.height;
  run.cells = src.cells.size();
  run.loader = src.loader;
  run.lossy = src.lossy;
  run.time_n = std::min(cfg.max_time_cells, static_cast<int>(src.cells.size()));

  if (!cfg.compare) {
    run.results.push_back(
        measure_codec(cfg.variants.front(), cfg.qualities, src, run.time_n, cfg.repeats));
    return;
  }
  for (const Variant& v : cfg.variants) {
    std::vector<int> qs = cfg.qualities;
    if (v == cfg.reference.variant &&
        std::find(qs.begin(), qs.end(), cfg.reference.quality) == qs.end()) {
      qs.push_back(cfg.reference.quality);  // the target must be measured
      std::sort(qs.begin(), qs.end());
    }
    run.results.push_back(measure_codec(v, qs, src, run.time_n, cfg.repeats));
  }
  // Refine every codec's match against the reference PSNR (see refine_match).
  double target = -1;
  for (const auto& r : run.results) {
    if (!(r.variant == cfg.reference.variant) || r.status != Status::Ok) continue;
    for (const Row& row : r.rows) {
      if (row.quality == cfg.reference.quality) target = row.psnr_db;
    }
  }
  if (target >= 0) {
    for (auto& r : run.results) {
      if (!(r.variant == cfg.reference.variant)) {
        refine_match(r, target, src, run.time_n, cfg.repeats);
      }
    }
  }
  run.outcome = evaluate_compare(run.results, cfg.reference, cfg.tie_pct);
}

/// Is `p` a raster image libvips can read? Decided by content, and limited to
/// the formats the tile codecs make sense for (not PDF, SVG, ...).
bool is_raster_image(const fs::path& p) {
  ensure_vips();
  const char* loader = vips_foreign_find_load(p.string().c_str());
  if (!loader) {
    vips_error_clear();
    return false;
  }
  const std::string name = loader;
  for (const char* prefix : {"VipsForeignLoadJpeg", "VipsForeignLoadPng", "VipsForeignLoadWebp",
                             "VipsForeignLoadHeif", "VipsForeignLoadJxl", "VipsForeignLoadTiff",
                             "VipsForeignLoadGif"}) {
    if (name.rfind(prefix, 0) == 0) return true;
  }
  return false;
}

// --- JSON documents -------------------------------------------------------------

/// Identity fields for a variant: base codec, plus variant/effort when set
/// (rows for default-effort codecs keep the original schema-1 keys).
void write_variant_json(gp::JsonWriter& w, const Variant& v) {
  w.field("codec", codec_name(v.codec));
  if (v.effort >= 0) {
    w.field("variant", v.name());
    w.field("effort", v.effort);
  }
}

void write_row_json(gp::JsonWriter& w, const Variant& v, const Row& row) {
  w.begin_object(gp::JsonWriter::Compact);
  write_variant_json(w, v);
  w.field("quality", row.quality);
  w.field("encode_ms", row.enc.median);
  w.field("decode_ms", row.dec.median);
  w.field("bytes_total", row.bytes_total);
  w.field("bytes_per_cell_mean", row.bytes_per_cell_mean);
  w.field("psnr_db", row.psnr_db);
  w.end_object();
}

void write_header_json(gp::JsonWriter& w, const FileRun& run, const Config& cfg) {
  w.field("tool", "thumtoo-gp-tile");
  w.field("file", run.path.string());
  w.field("width", run.width);
  w.field("height", run.height);
  w.field("tile", cfg.tile);
  w.field("cells", run.cells);
  w.field("timed_cells", run.time_n);
  w.field("repeats", cfg.repeats);
}

/// An image that could not be prepared: the same shape in every mode.
void write_load_failure_doc(gp::JsonWriter& w, const FileRun& run) {
  w.begin_object();
  w.field("schema", 1);
  w.field("tool", "thumtoo-gp-tile");
  w.field("file", run.path.string());
  w.field("status", "failed");
  w.field("reason", run.load_error);
  w.end_object();
}

/// One codec on one image. Keys are stable for checked-in baselines;
/// "status" (and "reason" instead of rows on failure) is additive.
void write_single_doc(gp::JsonWriter& w, const FileRun& run, const Config& cfg) {
  if (!run.loaded()) return write_load_failure_doc(w, run);
  const CodecResult& r = run.results.front();
  w.begin_object();
  w.field("schema", 1);
  write_header_json(w, run, cfg);
  w.field("status", status_name(r.status));
  if (r.status == Status::Ok) {
    w.key("rows").begin_array();
    for (const Row& row : r.rows) write_row_json(w, r.variant, row);
    w.end_array();
  } else {
    w.field("reason", r.reason);
  }
  w.end_object();
}

void write_compare_doc(gp::JsonWriter& w, const FileRun& run, const Config& cfg) {
  if (!run.loaded()) return write_load_failure_doc(w, run);
  const CompareOutcome& o = run.outcome;
  w.begin_object();
  w.field("schema", 1);
  w.field("kind", "compare");
  write_header_json(w, run, cfg);
  w.field("source_loader", run.loader);
  w.field("source_lossy", run.lossy);
  w.key("reference").begin_object(gp::JsonWriter::Compact);
  write_variant_json(w, cfg.reference.variant);
  w.field("quality", cfg.reference.quality);
  if (o.ref_row) w.field("psnr_db", o.target);
  else w.key("psnr_db").value(nullptr);
  w.end_object();

  w.key("codecs").begin_array();
  for (const CodecResult& r : run.results) {
    w.begin_object();
    write_variant_json(w, r.variant);
    w.field("status", status_name(r.status));
    if (r.status == Status::Ok) {
      w.key("rows").begin_array();
      for (const Row& row : r.rows) write_row_json(w, r.variant, row);
      w.end_array();
    } else {
      w.field("reason", r.reason);
    }
    w.end_object();
  }
  w.end_array();

  w.key("matched").begin_array();
  for (const Match& m : o.matches) {
    if (m.row) {
      write_row_json(w, m.variant, *m.row);
    } else {
      w.begin_object(gp::JsonWriter::Compact);
      write_variant_json(w, m.variant);
      w.field("below_target", true);
      w.field("best_psnr_db", m.best ? m.best->psnr_db : 0.0);
      w.end_object();
    }
  }
  w.end_array();

  w.key("verdict");
  gp::write_verdict_json(w, o.verdict);
  w.end_object();
}

// --- CSV rows -------------------------------------------------------------------

const std::vector<std::string>& csv_columns() {
  static const std::vector<std::string> cols = {
      "file",        "width",           "height",   "variant",       "codec",
      "effort",      "status",          "reason",   "quality",       "encode_ms",
      "decode_ms",   "bytes_total",     "bytes_per_cell_mean",       "psnr_db",
      "matched",     "overall_ratio"};
  return cols;
}

/// One row per measured (variant, quality); a variant that could not run, and
/// an image that could not be prepared, get one row with status and reason.
/// In a comparison, matched=1 marks the setting that represents the variant in
/// the verdict, and overall_ratio (1 = best) is filled on that row.
std::vector<std::vector<std::string>> csv_rows(const FileRun& run, const Config& cfg) {
  std::vector<std::vector<std::string>> out;
  const std::string file = run.path.string();
  if (!run.loaded()) {
    out.push_back({file, "", "", "", "", "", "failed", run.load_error, "", "", "", "", "", "", "", ""});
    return out;
  }
  const std::string w = std::to_string(run.width);
  const std::string h = std::to_string(run.height);
  for (const CodecResult& r : run.results) {
    const std::string effort = r.variant.effort >= 0 ? std::to_string(r.variant.effort) : "";
    if (r.status != Status::Ok) {
      out.push_back({file, w, h, r.variant.name(), codec_name(r.variant.codec), effort,
                     status_name(r.status), r.reason, "", "", "", "", "", "", "", ""});
      continue;
    }
    const Row* matched = nullptr;
    if (cfg.compare) {
      for (const Match& m : run.outcome.matches) {
        if (m.variant == r.variant) matched = m.row;
      }
    }
    for (const Row& row : r.rows) {
      std::string is_matched, ratio;
      if (cfg.compare) {
        is_matched = matched == &row ? "1" : "0";
        if (matched == &row && run.outcome.verdict.overall.size() > 1) {
          if (const auto v = run.outcome.verdict.overall_ratio(r.variant.name())) {
            ratio = gp::csv_num(*v);
          }
        }
      }
      out.push_back({file, w, h, r.variant.name(), codec_name(r.variant.codec), effort, "ok", "",
                     std::to_string(row.quality), gp::csv_num(row.enc.median),
                     gp::csv_num(row.dec.median), std::to_string(row.bytes_total),
                     gp::csv_num(row.bytes_per_cell_mean), gp::csv_num(row.psnr_db), is_matched,
                     ratio});
    }
  }
  return out;
}

// --- text report ----------------------------------------------------------------

gp::TextTable row_table() {
  using gp::Align;
  return gp::TextTable({{"variant", Align::Left},
                        {"quality", Align::Right},
                        {"encode ms", Align::Right},
                        {"decode ms", Align::Right},
                        {"size", Align::Right},
                        {"PSNR dB", Align::Right}});
}

std::vector<std::string> text_row(const Variant& v, const Row& row) {
  return {v.name(),
          std::to_string(row.quality),
          gp::fmt_ms(row.enc.median),
          gp::fmt_ms(row.dec.median),
          gp::fmt_bytes(row.bytes_total),
          gp::fmt_fixed(row.psnr_db, 2)};
}

void print_heading(const FileRun& run, const Config& cfg) {
  std::cout << run.name;
  if (run.loaded()) {
    std::cout << "  (" << run.width << "x" << run.height << ", " << run.cells << " tiles of "
              << cfg.tile << " px";
    // Time and size cover different tile sets only when --max-cells cut the timing.
    if (run.time_n < static_cast<int>(run.cells)) {
      std::cout << "; times are for " << run.time_n << " tiles, sizes for all";
    }
    std::cout << ")";
  }
  std::cout << "\n";
}

void print_block(const FileRun& run, const Config& cfg, bool sweep) {
  print_heading(run, cfg);
  if (!run.loaded()) {
    std::cout << "  failed — " << run.load_error << "\n\n";
    return;
  }

  if (!cfg.compare) {
    const CodecResult& r = run.results.front();
    if (r.status == Status::Ok) {
      gp::TextTable table = row_table();
      for (const Row& row : r.rows) table.add_row(text_row(r.variant, row));
      table.print(std::cout, "  ");
    } else {
      std::cout << "  " << r.variant.name() << ": " << status_name(r.status) << " — " << r.reason
                << "\n";
    }
    std::cout << "\n";
    return;
  }

  const CompareOutcome& o = run.outcome;
  if (sweep) {
    gp::TextTable all = row_table();
    for (const CodecResult& r : run.results) {
      if (r.status != Status::Ok) continue;
      for (const Row& row : r.rows) all.add_row(text_row(r.variant, row));
    }
    std::cout << "  every measured setting:\n";
    all.print(std::cout, "  ");
    std::cout << "\n";
  }
  for (const CodecResult& r : run.results) {
    if (r.status != Status::Ok) {
      std::cout << "  " << r.variant.name() << ": " << status_name(r.status) << " — " << r.reason
                << "\n";
    }
  }
  if (!o.ref_row) {
    std::cout << "  verdict refused: reference " << cfg.reference.name()
              << " was not measured successfully\n\n";
    return;
  }
  std::cout << "  target quality: " << cfg.reference.name() << " = "
            << gp::fmt_fixed(o.target, 2)
            << " dB PSNR; each variant at its smallest setting that reaches it:\n";
  gp::TextTable matched = row_table();
  for (const Match& m : o.matches) {
    if (m.row) matched.add_row(text_row(m.variant, *m.row));
  }
  if (!matched.empty()) matched.print(std::cout, "  ");
  for (const Match& m : o.matches) {
    if (!m.row) {
      std::cout << "  " << m.variant.name() << ": below target — best "
                << gp::fmt_fixed(m.best ? m.best->psnr_db : 0.0, 2) << " dB at q"
                << (m.best ? m.best->quality : 0) << " in this sweep\n";
    }
  }
  std::ostringstream verdict;
  gp::print_verdict(verdict, o.verdict);
  std::cout << "\n" << gp::indent_lines(verdict.str(), "  ") << "\n";
}

// --- reporter -------------------------------------------------------------------

/// Receives each image as it finishes and renders it in the chosen format.
class Reporter {
 public:
  Reporter(gp::OutputMode mode, const Config& cfg, bool csv_header, bool sweep,
           std::size_t total)
      : mode_(mode),
        cfg_(cfg),
        sweep_(sweep),
        total_(total),
        agg_(matched_metrics(), cfg.tie_pct) {
    if (mode_ == gp::OutputMode::Csv) {
      csv_.emplace(std::cout, csv_columns(), csv_header);
    } else if (mode_ == gp::OutputMode::Json && total_ > 1) {
      json_.emplace(std::cout);
      json_->begin_object();
      json_->field("schema", 1);
      json_->field("kind", "batch");
      json_->field("tool", "thumtoo-gp-tile");
      json_->field("repeats", cfg_.repeats);
      json_->key("results").begin_array();
    }
  }

  void add(const FileRun& run) {
    if (!run.ok(cfg_.compare)) ++failures_;
    if (cfg_.compare && run.loaded()) agg_.add(run.outcome.verdict);
    switch (mode_) {
      case gp::OutputMode::Text:
        print_block(run, cfg_, sweep_);
        break;
      case gp::OutputMode::Csv:
        for (const auto& row : csv_rows(run, cfg_)) csv_->row(row);
        break;
      case gp::OutputMode::Json:
        if (json_) {
          emit_json(*json_, run);
        } else {
          gp::JsonWriter w(std::cout);  // a single image: the bare document
          emit_json(w, run);
        }
        break;
    }
  }

  /// Trailing output; returns the process exit status (1 if anything failed).
  int finish() {
    if (mode_ == gp::OutputMode::Text && cfg_.compare && total_ > 1) {
      gp::print_aggregate(std::cout, agg_.result(), "images");
    } else if (mode_ == gp::OutputMode::Json && json_) {
      json_->end_array();
      if (cfg_.compare) {
        json_->key("aggregate");
        gp::write_aggregate_json(*json_, agg_.result());
      }
      json_->end_object();
    }
    if (failures_ > 0) {
      std::cerr << "thumtoo-gp-tile: " << failures_ << " of " << total_
                << (total_ == 1 ? " image" : " images")
                << (cfg_.compare ? " could not be compared" : " could not be measured") << "\n";
      return 1;
    }
    return 0;
  }

 private:
  void emit_json(gp::JsonWriter& w, const FileRun& run) const {
    if (cfg_.compare) write_compare_doc(w, run, cfg_);
    else write_single_doc(w, run, cfg_);
  }

  gp::OutputMode mode_;
  const Config& cfg_;
  bool sweep_;
  std::size_t total_;
  gp::Aggregator agg_;
  std::optional<gp::CsvWriter> csv_;
  std::optional<gp::JsonWriter> json_;
  std::size_t failures_ = 0;
};

// --- command line ---------------------------------------------------------------

/// "jpeg", "all", "jxl@e1", or a comma-separated mix. "all" expands in place,
/// so "all,jxl@e1" adds a fast JXL to the defaults.
std::optional<std::vector<Variant>> parse_codec_list(const std::string& flag, std::string& err) {
  std::vector<Variant> variants;
  for (const std::string& part : gp::cli::split(flag, ',')) {
    std::vector<Variant> add;
    if (part == "all") {
      for (Codec c : kAllCodecs) add.push_back(Variant{c, -1});
    } else {
      const auto v = parse_variant(part, err);
      if (!v) return std::nullopt;
      add.push_back(*v);
    }
    for (const Variant& v : add) {
      if (std::find(variants.begin(), variants.end(), v) == variants.end()) variants.push_back(v);
    }
  }
  if (variants.empty()) {
    err = "expected a codec, 'all', or a comma-separated list";
    return std::nullopt;
  }
  return variants;
}

gp::cli::Spec make_spec() {
  gp::cli::Spec s;
  s.program = "thumtoo-gp-tile";
  s.synopsis = "[OPTION]... FILE|DIR...";
  s.summary =
      "Cut an image into square tiles, encode and decode every tile with one or more "
      "codecs (JPEG, WebP, AVIF, JPEG XL through libvips) and report encode time, "
      "decode time, size and PSNR at several quality settings. With more than one "
      "codec the tool compares them at equal image quality and says which wins.\n\n"
      "Codecs are never compared at the same quality number: JPEG 80 and AVIF 80 mean "
      "different things. Instead the PSNR of a reference setting (default jpeg:80, "
      "thumtoo's tile default) is the target, and each codec is shown at the smallest "
      "setting that reaches it. Use a lossless source such as PNG: re-encoding a JPEG "
      "favors JPEG. Which codecs libvips can actually encode is checked at runtime; "
      "one it cannot is reported as unsupported, never as a zero-byte result.";
  s.options = {
      {"codec", 'c', "LIST",
       "Codecs to run: jpeg (the default), webp, avif, jxl, all, or a comma-separated "
       "list. Add an encoder effort with @eN: webp@e0..6, avif@e0..9, jxl@e1..9, e.g. "
       "jxl@e1; without it libvips' default is used (webp 4, avif 4, jxl 7). jpeg has no "
       "effort setting. 'all' may be combined: all,jxl@e1. More than one entry is a "
       "comparison."},
      {"quality", 'q', "LIST",
       "Quality settings to sweep, 1-100, comma separated. Default: 60,80,90 for one "
       "codec; 30,40,...,90 for a comparison."},
      {"reference", 0, "VARIANT:Q",
       "Comparison only: the setting whose PSNR is the quality target, e.g. jpeg:80 "
       "(the default) or jxl@e3:75. It must be one of the --codec entries."},
      {"tie-pct", 0, "PCT",
       "Comparison only: results within PCT percent of the best, or within run-to-run "
       "noise, count as a tie, 0-100 (default 5)."},
      {"tile", 't', "N", "Tile edge in pixels, 16-4096 (default 256)."},
      {"repeat", 'n', "N", "Timed runs per measurement, 1-1000 (default 3)."},
      {"max-cells", 0, "K",
       "Time at most K tiles per image (default 16). Size and PSNR always cover every "
       "tile."},
      {"recursive", 'r', "",
       "Search subdirectories of any DIR as well. Directories are searched for raster "
       "images (JPEG, PNG, WebP, AVIF/HEIF, JPEG XL, TIFF, GIF) by content, sorted by "
       "name."},
      {"sweep", 0, "",
       "Comparison, text output: also list every measured setting, not only each "
       "codec's matched one."},
  };
  for (const auto& o : gp::output_options()) s.options.push_back(o);
  s.sections = {
      {"Output formats",
       "Default: aligned tables and a verdict in words, for reading. With several\n"
       "images and a comparison, a summary over all images follows.\n"
       "--csv:   one row per image, codec variant and quality; raw milliseconds and\n"
       "         bytes; empty cell = not measured. In a comparison matched=1 marks the\n"
       "         setting that represents the variant, with overall_ratio (1 = best).\n"
       "--json:  one document for one image; with several images a batch document\n"
       "         {kind: \"batch\", results: [...], aggregate: {...}}.\n"
       "Warnings and errors go to stderr, so stdout is clean for pipes."},
      {"Exit status",
       "0  every image was measured (comparison: and a verdict could be reached)\n"
       "1  an image could not be read, or no verdict could be reached\n"
       "2  usage error"},
      {"Examples",
       "thumtoo-gp-tile photo.png\n"
       "thumtoo-gp-tile --codec all photo.png\n"
       "thumtoo-gp-tile -c all,jxl@e1,jxl@e3 --reference jpeg:85 corpus/synthetic/png/\n"
       "thumtoo-gp-tile --csv -c jpeg,webp,jxl@e3 *.png > codecs.csv\n"
       "thumtoo-gp-tile --json -c all photo.png"},
  };
  return s;
}

}  // namespace

int main(int argc, char** argv) {
  namespace cli = gp::cli;
  const cli::Spec spec = make_spec();
  cli::Args args;
  switch (cli::parse(spec, argc, argv, args)) {
    case cli::Parsed::Help:
      cli::print_help(std::cout, spec);
      return 0;
    case cli::Parsed::Version:
      cli::print_version(std::cout, spec.program);
      return 0;
    case cli::Parsed::Error:
      return cli::fail_usage(spec, args.errors());
    case cli::Parsed::Run:
      break;
  }

  Config cfg;
  cfg.tile = args.get_int("tile", 256, 16, 4096);
  cfg.repeats = args.get_int("repeat", 3, 1, 1000);
  cfg.max_time_cells = args.get_int("max-cells", 16, 1, 1000000);
  cfg.tie_pct = args.get_double("tie-pct", 5.0, 0.0, 100.0);
  const gp::OutputMode mode = gp::resolve_output_mode(args);

  std::string codec_err;
  const auto variants = parse_codec_list(args.get_string("codec", "jpeg"), codec_err);
  if (!variants) {
    args.add_error("--codec: " + codec_err);
  } else {
    cfg.variants = *variants;
    cfg.compare = cfg.variants.size() > 1;
  }
  std::string ref_err;
  const auto reference = parse_reference(args.get_string("reference", "jpeg:80"), ref_err);
  if (!reference) {
    args.add_error("--reference: " + ref_err + " (expected VARIANT:Q, e.g. jpeg:80)");
  } else {
    cfg.reference = *reference;
    if (cfg.compare && std::find(cfg.variants.begin(), cfg.variants.end(),
                                 cfg.reference.variant) == cfg.variants.end()) {
      args.add_error("--reference " + cfg.reference.name() + " is not one of the --codec "
                     "entries; add " + cfg.reference.variant.name() +
                     " to --codec or choose another reference");
    }
  }
  cfg.qualities = args.get_int_list("quality", {}, 1, 100);
  if (cfg.qualities.empty()) {
    cfg.qualities = cfg.compare ? std::vector<int>{30, 40, 50, 60, 70, 80, 90}
                                : std::vector<int>{60, 80, 90};
  }

  if (args.positionals().empty()) args.add_error("missing FILE argument");
  std::vector<std::string> input_errors;
  const std::vector<fs::path> inputs =
      cli::expand_inputs(args.positionals(), args.has("recursive"), is_raster_image, input_errors);
  for (const std::string& e : input_errors) args.add_error(e);
  if (args.errors().empty() && inputs.empty()) args.add_error("no images to measure");
  if (!args.errors().empty()) return cli::fail_usage(spec, args.errors());

  const std::vector<std::string> names = gp::display_names(inputs);
  Reporter reporter(mode, cfg, !args.has("no-header"), args.has("sweep"), inputs.size());
  cli::Progress progress;
  for (std::size_t i = 0; i < inputs.size(); ++i) {
    progress.update(i + 1, inputs.size(), names[i]);
    FileRun run;
    run.path = inputs[i];
    run.name = names[i];
    measure_file(run, cfg);
    progress.clear();
    if (cfg.compare && run.lossy) {
      std::cerr << "warning: " << run.name << " is a lossy source (" << run.loader
                << "); that biases the comparison toward its own codec — prefer a PNG source\n";
    }
    reporter.add(run);
  }
  return reporter.finish();
}
