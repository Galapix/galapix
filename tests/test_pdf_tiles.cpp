// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/// Integration tests for PDF interactive (live) tiles and negative scales.
///
/// Contract under test (see TILES.md):
/// * Interactive request_tile for //page:N always replies with codec=rgb888
///   raw pixels (never JPEG in the callback payload).
/// * Durable store (JPEG) only when scale >= kPdfMinDurableTileScale.
/// * Negative scale uses dpi = kPdfLayoutDpi * 2^{-scale} and region crop.
/// * Cell dimensions are min(kTileSize, remaining) on the scale grid.
/// * Cached get_tile returns the stored JPEG (codec=jpeg) after a durable hit.

#include "thumtoo/client.hpp"
#include "thumtoo/constants.hpp"
#include "thumtoo/pdf.hpp"
#include "thumtoo/uri.hpp"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;

namespace {

int g_failures = 0;

void expect(bool cond, const char* msg) {
  if (!cond) {
    std::cerr << "FAIL: " << msg << "\n";
    ++g_failures;
  }
}

void expect_eq(int a, int b, const char* msg) {
  if (a != b) {
    std::cerr << "FAIL: " << msg << " (" << a << " != " << b << ")\n";
    ++g_failures;
  }
}

void expect_eq_str(const std::string& a, const std::string& b, const char* msg) {
  if (a != b) {
    std::cerr << "FAIL: " << msg << " (\"" << a << "\" != \"" << b << "\")\n";
    ++g_failures;
  }
}

/// Self-contained letter-page PDF (no Ghostscript / external tools).
/// MediaBox 612×792 pt → layout 1224×1584 @ 144 dpi.
/// Text-only content: a Vector page, so every live scale renders.
std::optional<fs::path> make_test_pdf(const fs::path& dir) {
  // Build a content stream with ~1600 printable chars (40 lines × ~40).
  std::string body;
  body.reserve(4096);
  for (int i = 0; i < 40; ++i) {
    body += "BT /F1 10 Tf 36 ";
    body += std::to_string(750 - i * 18);
    body += " Td (Line ";
    if (i < 10) body += '0';
    body += std::to_string(i);
    body += " ABCDEFGHIJKLMNOPQRSTUVWXYZ012345) Tj ET\n";
  }

  std::string pdf;
  pdf.reserve(body.size() + 512);
  auto append = [&](std::string_view s) { pdf.append(s); };

  // Object offsets recorded as we append (byte offset from file start).
  std::vector<std::size_t> obj_off(6, 0);
  append("%PDF-1.4\n");
  obj_off[1] = pdf.size();
  append("1 0 obj<< /Type /Catalog /Pages 2 0 R >>endobj\n");
  obj_off[2] = pdf.size();
  append("2 0 obj<< /Type /Pages /Kids [3 0 R] /Count 1 >>endobj\n");
  obj_off[3] = pdf.size();
  append("3 0 obj<< /Type /Page /Parent 2 0 R /MediaBox [0 0 612 792] "
         "/Contents 4 0 R /Resources<< /Font<< /F1 5 0 R >> >> >>endobj\n");
  obj_off[4] = pdf.size();
  append("4 0 obj<< /Length ");
  append(std::to_string(body.size()));
  append(" >>stream\n");
  append(body);
  append("endstream\nendobj\n");
  obj_off[5] = pdf.size();
  append("5 0 obj<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>endobj\n");
  const std::size_t xref_pos = pdf.size();
  append("xref\n0 6\n");
  append("0000000000 65535 f \n");
  for (int i = 1; i <= 5; ++i) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%010zu 00000 n \n", obj_off[static_cast<std::size_t>(i)]);
    append(buf);
  }
  append("trailer<< /Size 6 /Root 1 0 R >>\nstartxref\n");
  append(std::to_string(xref_pos));
  append("\n%%EOF\n");

  const fs::path path = dir / "tile-test.pdf";
  std::ofstream out(path, std::ios::binary);
  if (!out) return std::nullopt;
  out.write(pdf.data(), static_cast<std::streamsize>(pdf.size()));
  out.close();
  if (!fs::is_regular_file(path) || fs::file_size(path) < 100) return std::nullopt;
  return path;
}

std::optional<thumtoo::TileBlob> request_one(thumtoo::Client& client,
                                             const std::string& uri, int scale,
                                             int x, int y) {
  std::optional<thumtoo::TileBlob> out;
  bool done = false;
  client.request_tile(uri, scale, x, y,
                      [&](std::string, int, int, int,
                          std::optional<thumtoo::TileBlob> tb) {
                        out = std::move(tb);
                        done = true;
                      });
  client.drain();
  expect(done, "request_tile callback fired");
  return out;
}

}  // namespace

int main() {
#if !defined(THUMTOO_HAVE_MUPDF)
  std::cout << "test_pdf_tiles: skip (no MuPDF)\n";
  return 0;
#endif

  const fs::path work = fs::temp_directory_path() / "thumtoo-test-pdf-tiles";
  std::error_code ec;
  fs::remove_all(work, ec);
  fs::create_directories(work, ec);
  if (ec) {
    std::cerr << "cannot create work dir\n";
    return 1;
  }

  auto pdf_path = make_test_pdf(work);
  if (!pdf_path) {
    std::cerr << "FAIL: could not create test PDF (need gs)\n";
    return 1;
  }

  const std::string uri = thumtoo::pdf_page_uri(*pdf_path, 1);
  const fs::path cache = work / "cache";

  auto client = thumtoo::Client::open(cache, {}, 0, cache);
  expect(static_cast<bool>(client), "Client::open");
  if (!client) return 1;

  // Probe size (layout @ kPdfLayoutDpi).
  {
    bool done = false;
    client->request_size(uri, [&](std::string, thumtoo::SizeReply) {
      done = true;
    });
    client->drain();
    expect(done, "size probe completed");
  }

  auto sz = client->get_size(uri);
  expect(static_cast<bool>(sz), "get_size after probe");
  if (!sz) return 1;
  expect(sz->width > 0 && sz->height > 0, "layout size positive");

  // Geometry helpers match TILES.md formulas.
  expect_eq(thumtoo::kPdfLayoutDpi, 144, "layout dpi constant");
  expect_eq(thumtoo::kPdfMinDurableTileScale, -2, "durable floor scale");
  {
    auto f0 = thumtoo::pdf_page_size_at_scale(*sz, 0);
    expect_eq(f0.width, sz->width, "scale0 width == layout");
    auto fm1 = thumtoo::pdf_page_size_at_scale(*sz, -1);
    expect_eq(fm1.width, sz->width * 2, "scale-1 width == 2*layout");
    expect(std::abs(thumtoo::pdf_dpi_for_scale(-1) - 288.0) < 1e-6,
           "dpi scale-1 == 288");
    expect(std::abs(thumtoo::pdf_dpi_for_scale(-3) - 1152.0) < 1e-6,
           "dpi scale-3 == 1152");
  }

  const int T = thumtoo::kTileSize;

  // --- Live interactive path: scale 0 (durable floor, still rgb888 in reply) ---
  {
    auto live = request_one(*client, uri, 0, 0, 0);
    expect(static_cast<bool>(live) && live && !live->bytes.empty(),
           "live scale0 non-empty");
    if (live) {
      expect_eq_str(live->codec, thumtoo::kTileCodecRgb888,
                    "live scale0 codec is rgb888");
      expect_eq(live->width, T, "live scale0 width");
      expect_eq(live->height, T, "live scale0 height");
      expect_eq(static_cast<int>(live->bytes.size()), T * T * 3,
                "live scale0 byte count");
      expect_eq(live->scale, 0, "live scale0 meta scale");
    }
    // Durable JPEG should now exist for scale 0.
    expect(client->has_tile(uri, 0, 0, 0), "scale0 stored in durable cache");
    auto cached = client->get_tile(uri, 0, 0, 0);
    expect(static_cast<bool>(cached), "get_tile scale0 after store");
    if (cached) {
      expect_eq_str(cached->codec, "jpeg", "cached scale0 codec is jpeg");
      expect(!cached->bytes.empty(), "cached scale0 has JPEG bytes");
      // JPEG payload must not equal raw RGB size (or be identical to rgb888).
      expect(cached->bytes.size() != live->bytes.size() ||
                 cached->codec != live->codec,
             "cached payload differs from live rgb888");
    }
  }

  // --- Negative scale within durable floor (-1, -2): reply rgb888, store JPEG ---
  for (int scale : {-1, -2}) {
    auto live = request_one(*client, uri, scale, 0, 0);
    expect(static_cast<bool>(live) && live && !live->bytes.empty(),
           (std::string("live scale") + std::to_string(scale) + " non-empty")
               .c_str());
    if (!live) continue;
    expect_eq_str(live->codec, thumtoo::kTileCodecRgb888,
                  (std::string("live scale") + std::to_string(scale) +
                   " codec rgb888")
                      .c_str());
    expect_eq(live->width, T, "neg scale full-tile width");
    expect_eq(live->height, T, "neg scale full-tile height");
    expect_eq(static_cast<int>(live->bytes.size()), T * T * 3,
              "neg scale byte count");
    expect(client->has_tile(uri, scale, 0, 0),
           (std::string("scale") + std::to_string(scale) + " durable stored")
               .c_str());
    auto cached = client->get_tile(uri, scale, 0, 0);
    expect(static_cast<bool>(cached) && cached && cached->codec == "jpeg",
           (std::string("cached scale") + std::to_string(scale) + " is jpeg")
               .c_str());
  }

  // --- Below durable floor (scale -3): live-only, no store ---
  {
    const int scale = -3;
    // Ensure not already present.
    client->invalidate_tile(uri, scale, 0, 0);
    expect(!client->has_tile(uri, scale, 0, 0), "scale-3 not cached before");
    auto live = request_one(*client, uri, scale, 0, 0);
    expect(static_cast<bool>(live) && live && !live->bytes.empty(),
           "live scale-3 non-empty");
    if (live) {
      expect_eq_str(live->codec, thumtoo::kTileCodecRgb888,
                    "live scale-3 codec rgb888");
      expect_eq(live->width, T, "scale-3 width");
      expect_eq(live->height, T, "scale-3 height");
    }
    expect(!client->has_tile(uri, scale, 0, 0),
           "scale-3 must NOT be written to durable cache");
    auto cached = client->get_tile(uri, scale, 0, 0);
    expect(!cached, "get_tile scale-3 remains miss");
  }

  // --- Edge tile (partial width/height) at scale 0 ---
  {
    const auto full = thumtoo::pdf_page_size_at_scale(*sz, 0);
    const int tiles_w = (full.width + T - 1) / T;
    const int tiles_h = (full.height + T - 1) / T;
    expect(tiles_w >= 1 && tiles_h >= 1, "at least one tile");
    const int ex = tiles_w - 1;
    const int ey = tiles_h - 1;
    const int expect_w = full.width - ex * T;
    const int expect_h = full.height - ey * T;
    auto live = request_one(*client, uri, 0, ex, ey);
    expect(static_cast<bool>(live), "edge tile live");
    if (live) {
      expect_eq(live->width, expect_w, "edge tile width");
      expect_eq(live->height, expect_h, "edge tile height");
      expect_eq(static_cast<int>(live->bytes.size()), expect_w * expect_h * 3,
                "edge tile rgb bytes");
      expect_eq_str(live->codec, thumtoo::kTileCodecRgb888, "edge codec");
    }
  }

  // --- Out-of-range tile indices → empty ---
  {
    auto live = request_one(*client, uri, 0, 9999, 9999);
    expect(!live || live->bytes.empty(), "out-of-range tile is empty");
  }

  // --- Direct pdf_render_tile_cell matches Client live path dimensions ---
  {
    auto cell = thumtoo::pdf_render_tile_cell(*pdf_path, 1, -1, 0, 0);
    auto& direct = cell.raster;
    expect(static_cast<bool>(direct), "pdf_render_tile_cell scale-1");
    if (direct) {
      expect_eq(direct->width, T, "direct cell width");
      expect_eq(direct->height, T, "direct cell height");
      expect_eq(static_cast<int>(direct->rgb.size()), T * T * 3,
                "direct cell rgb size");
    }
  }

  // --- Coverage reports only durable scales (not negative live-only) ---
  {
    auto cov = client->get_tile_coverage(uri);
    expect(static_cast<bool>(cov), "coverage after durable tiles");
    if (cov) {
      // We stored 0, -1, -2 — min can be -2.
      expect(cov->min_scale <= 0, "coverage min_scale <= 0");
      expect(cov->min_scale >= thumtoo::kPdfMinDurableTileScale,
             "coverage min does not go below durable floor from our stores");
    }
  }

  thumtoo::pdf_release_document_cache();
  // Cleanup
  fs::remove_all(work, ec);

  if (g_failures) {
    std::cerr << "test_pdf_tiles: " << g_failures << " failure(s)\n";
    return 1;
  }
  std::cout << "test_pdf_tiles: ok\n";
  return 0;
}
