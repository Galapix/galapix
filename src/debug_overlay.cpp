// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "thumtoo/debug_overlay.hpp"

#include "thumtoo/constants.hpp"
#include "thumtoo/image.hpp"

#include <vips/vips.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>

namespace thumtoo {
namespace {

bool env_flag_on(const char* name) {
  const char* v = std::getenv(name);
  if (!v || !v[0] || v[0] == '0') {
    return false;
  }
  return true;
}

// 8×12 bitmap font for DEBUG_OVERLAY stamps (no fontconfig / Pango).
// Each glyph is 8 columns × 12 rows; bit 7 = leftmost pixel.
// Designed for TILE / SOFT / s=N / x,y labels; scales cleanly via nearest-neighbour.
constexpr int kGw = 8;
constexpr int kGh = 12;
constexpr int kGGap = 2;

std::uint8_t glyph_row(char c, int row) {
  static const std::uint8_t digits[10][12] = {
      {0x3c, 0x66, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0x66, 0x3c}, // 0
      {0x18, 0x38, 0x78, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x7e}, // 1
      {0x7e, 0xc3, 0x03, 0x03, 0x06, 0x0c, 0x18, 0x30, 0x60, 0xc0, 0xc0, 0xff}, // 2
      {0x7e, 0xc3, 0x03, 0x03, 0x1e, 0x03, 0x03, 0x03, 0x03, 0xc3, 0xc3, 0x7e}, // 3
      {0x0c, 0x1c, 0x2c, 0x4c, 0xcc, 0xcc, 0xff, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c}, // 4
      {0xff, 0xc0, 0xc0, 0xc0, 0xfe, 0x03, 0x03, 0x03, 0x03, 0xc3, 0xc3, 0x7e}, // 5
      {0x3c, 0x66, 0xc0, 0xc0, 0xde, 0xe3, 0xc3, 0xc3, 0xc3, 0xc3, 0x66, 0x3c}, // 6
      {0xff, 0x03, 0x06, 0x06, 0x0c, 0x0c, 0x18, 0x18, 0x30, 0x30, 0x60, 0x60}, // 7
      {0x7e, 0xc3, 0xc3, 0xc3, 0x66, 0x3c, 0x66, 0xc3, 0xc3, 0xc3, 0xc3, 0x7e}, // 8
      {0x7e, 0xc3, 0xc3, 0xc3, 0xc3, 0x67, 0x3f, 0x03, 0x03, 0x03, 0x66, 0x3c}, // 9
  };
  if (row < 0 || row >= kGh) {
    return 0;
  }
  if (c >= '0' && c <= '9') {
    return digits[c - '0'][row];
  }
  if (c >= 'a' && c <= 'z') {
    c = static_cast<char>(c - 'a' + 'A');
  }
  switch (c) {
    case ' ':
      return 0;
    case '.':
      return (row == 9 || row == 10) ? 0x18 : 0;
    case ',':
      return row == 9    ? 0x18
             : row == 10 ? 0x18
             : row == 11 ? 0x30
                         : 0;
    case ':':
      return (row == 3 || row == 4 || row == 7 || row == 8) ? 0x18 : 0;
    case '=':
      return (row == 4 || row == 5 || row == 7 || row == 8) ? 0xff : 0;
    case '-':
      return (row == 5 || row == 6) ? 0xff : 0;
    case '_':
      return (row == 10 || row == 11) ? 0xff : 0;
    case '/': {
      // Diagonal from upper-right toward lower-left across 8 columns.
      static const std::uint8_t slash[12] = {0x03, 0x06, 0x06, 0x0c, 0x0c, 0x18,
                                             0x18, 0x30, 0x30, 0x60, 0x60, 0xc0};
      return slash[row];
    }
    case 'T':
      return row < 2 ? 0xff : 0x18;
    case 'I':
      return (row < 2 || row >= 10) ? 0x7e : 0x18;
    case 'L':
      return row >= 10 ? 0xff : 0xc0;
    case 'E':
      return (row < 2 || row >= 10) ? 0xff
             : (row == 5 || row == 6)  ? 0xfe
                                      : 0xc0;
    case 'S': {
      static const std::uint8_t s[12] = {0x7e, 0xc3, 0xc0, 0xc0, 0x60, 0x3c,
                                         0x0c, 0x06, 0x03, 0x03, 0xc3, 0x7e};
      return s[row];
    }
    case 'O':
      return digits[0][row];
    case 'F':
      return (row < 2)              ? 0xff
             : (row == 5 || row == 6) ? 0xfe
                                     : 0xc0;
    case 'A': {
      static const std::uint8_t a[12] = {0x18, 0x3c, 0x66, 0xc3, 0xc3, 0xc3,
                                         0xff, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3};
      return a[row];
    }
    case 'B': {
      static const std::uint8_t b[12] = {0xfe, 0xc3, 0xc3, 0xc3, 0xc3, 0xfe,
                                         0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xfe};
      return b[row];
    }
    case 'C': {
      static const std::uint8_t cc[12] = {0x3c, 0x66, 0xc3, 0xc0, 0xc0, 0xc0,
                                          0xc0, 0xc0, 0xc0, 0xc3, 0x66, 0x3c};
      return cc[row];
    }
    case 'D': {
      static const std::uint8_t d[12] = {0xfc, 0xc6, 0xc3, 0xc3, 0xc3, 0xc3,
                                         0xc3, 0xc3, 0xc3, 0xc3, 0xc6, 0xfc};
      return d[row];
    }
    case 'G': {
      static const std::uint8_t g[12] = {0x3c, 0x66, 0xc3, 0xc0, 0xc0, 0xcf,
                                         0xc3, 0xc3, 0xc3, 0xc3, 0x66, 0x3c};
      return g[row];
    }
    case 'H':
      return row == 5 || row == 6 ? 0xff : 0xc3;
    case 'J': {
      static const std::uint8_t j[12] = {0x1f, 0x06, 0x06, 0x06, 0x06, 0x06,
                                         0x06, 0x06, 0x06, 0xc6, 0xc6, 0x7c};
      return j[row];
    }
    case 'K': {
      static const std::uint8_t k[12] = {0xc3, 0xc6, 0xcc, 0xd8, 0xf0, 0xe0,
                                         0xf0, 0xd8, 0xcc, 0xc6, 0xc3, 0xc3};
      return k[row];
    }
    case 'M': {
      static const std::uint8_t m[12] = {0xc3, 0xe7, 0xff, 0xdb, 0xdb, 0xc3,
                                         0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3};
      return m[row];
    }
    case 'N': {
      static const std::uint8_t n[12] = {0xc3, 0xe3, 0xe3, 0xf3, 0xdb, 0xdb,
                                         0xcf, 0xc7, 0xc7, 0xc3, 0xc3, 0xc3};
      return n[row];
    }
    case 'P': {
      static const std::uint8_t p[12] = {0xfe, 0xc3, 0xc3, 0xc3, 0xc3, 0xfe,
                                         0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0};
      return p[row];
    }
    case 'R': {
      static const std::uint8_t r[12] = {0xfe, 0xc3, 0xc3, 0xc3, 0xc3, 0xfe,
                                         0xf0, 0xd8, 0xcc, 0xc6, 0xc3, 0xc3};
      return r[row];
    }
    case 'U': {
      static const std::uint8_t u[12] = {0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3,
                                         0xc3, 0xc3, 0xc3, 0xc3, 0x66, 0x3c};
      return u[row];
    }
    case 'V': {
      static const std::uint8_t v[12] = {0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3,
                                         0xc3, 0x66, 0x66, 0x3c, 0x3c, 0x18};
      return v[row];
    }
    case 'W': {
      static const std::uint8_t w[12] = {0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3,
                                         0xdb, 0xdb, 0xff, 0xe7, 0xc3, 0xc3};
      return w[row];
    }
    case 'X': {
      static const std::uint8_t x[12] = {0xc3, 0xc3, 0x66, 0x66, 0x3c, 0x18,
                                         0x18, 0x3c, 0x66, 0x66, 0xc3, 0xc3};
      return x[row];
    }
    case 'Y': {
      static const std::uint8_t y[12] = {0xc3, 0xc3, 0x66, 0x66, 0x3c, 0x18,
                                         0x18, 0x18, 0x18, 0x18, 0x18, 0x18};
      return y[row];
    }
    case 'Z': {
      static const std::uint8_t z[12] = {0xff, 0x03, 0x06, 0x0c, 0x18, 0x18,
                                         0x30, 0x30, 0x60, 0xc0, 0xc0, 0xff};
      return z[row];
    }
    case 'Q': {
      static const std::uint8_t q[12] = {0x3c, 0x66, 0xc3, 0xc3, 0xc3, 0xc3,
                                         0xc3, 0xdb, 0xcf, 0x66, 0x3c, 0x07};
      return q[row];
    }
    default:
      // Unknown → hollow box so missing glyphs are obvious.
      return (row == 0 || row == 11) ? 0xff : 0x81;
  }
}

void put_px(std::uint8_t* rgb, int w, int h, int x, int y, std::uint8_t r,
            std::uint8_t g, std::uint8_t b) {
  if (x < 0 || y < 0 || x >= w || y >= h) {
    return;
  }
  std::uint8_t* p = rgb + (static_cast<std::size_t>(y) * static_cast<std::size_t>(w) +
                           static_cast<std::size_t>(x)) *
                              3u;
  p[0] = r;
  p[1] = g;
  p[2] = b;
}

void draw_char_pass(std::uint8_t* rgb, int w, int h, int x0, int y0, char c,
                    int scale, int outline_px, bool outline) {
  for (int row = 0; row < kGh; ++row) {
    const std::uint8_t bits = glyph_row(c, row);
    for (int col = 0; col < kGw; ++col) {
      // Bit 7 = leftmost column.
      if (!(bits & (0x80 >> col))) {
        continue;
      }
      for (int dy = 0; dy < scale; ++dy) {
        for (int dx = 0; dx < scale; ++dx) {
          const int x = x0 + col * scale + dx;
          const int y = y0 + row * scale + dy;
          if (outline) {
            // Thick black halo before yellow fill (readable on any photo).
            for (int oy = -outline_px; oy <= outline_px; ++oy) {
              for (int ox = -outline_px; ox <= outline_px; ++ox) {
                if (ox == 0 && oy == 0) {
                  continue;
                }
                put_px(rgb, w, h, x + ox, y + oy, 0, 0, 0);
              }
            }
          } else {
            // Bright yellow fill (final pass).
            put_px(rgb, w, h, x, y, 255, 230, 40);
          }
        }
      }
    }
  }
}

void draw_text_line(std::uint8_t* rgb, int w, int h, int x0, int y0,
                    const std::string& s, int scale) {
  // Outline thickness grows with scale so large stamps keep a clear edge.
  const int outline_px = std::max(1, (scale + 2) / 3);
  // Two passes so outline of one glyph pixel cannot erase fill of another.
  auto run = [&](bool outline) {
    int x = x0;
    for (char c : s) {
      draw_char_pass(rgb, w, h, x, y0, c, scale, outline_px, outline);
      x += (kGw + kGGap) * scale;
      if (x >= w - 4) {
        break;
      }
    }
  };
  run(true);
  run(false);
}

void draw_debug_border(std::uint8_t* rgb, int w, int h, int thickness) {
  if (!rgb || w < 1 || h < 1 || thickness < 1) {
    return;
  }
  const int t = std::min(thickness, std::min(w, h) / 2);
  // Magenta border — visible on light and dark photos.
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      if (x < t || y < t || x >= w - t || y >= h - t) {
        put_px(rgb, w, h, x, y, 255, 0, 255);
      }
    }
  }
}

std::string basename_tail(std::string_view uri) {
  std::string s(uri);
  // Keep last path segment; strip query.
  const auto q = s.find('?');
  if (q != std::string::npos) {
    s.resize(q);
  }
  const auto slash = s.find_last_of("/\\");
  if (slash != std::string::npos) {
    s = s.substr(slash + 1);
  }
  if (s.size() > 28) {
    s = s.substr(0, 28);
  }
  return s;
}

bool rgb_from_encoded(const std::vector<std::uint8_t>& bytes, int& w, int& h,
                      std::vector<std::uint8_t>& out_rgb) {
  image_library_init();
  if (bytes.empty()) {
    return false;
  }
  VipsImage* img = vips_image_new_from_buffer(
      bytes.data(), static_cast<size_t>(bytes.size()), "", nullptr);
  // Soft ladder is often JXL — some libvips builds need an explicit loader.
  if (!img && bytes.size() >= 12) {
    (void)vips_jxlload_buffer(const_cast<void*>(static_cast<const void*>(bytes.data())),
                              bytes.size(), &img, nullptr);
  }
  if (!img) {
    // Live tiles are often raw rgb888 (e.g. 256×256×3 = 196608) — those must
    // not go through this path (see rgb_from_tile_blob). Log once per size.
    static std::mutex fail_mu;
    static std::size_t last_bytes = 0;
    static int fail_count = 0;
    {
      std::lock_guard<std::mutex> lock(fail_mu);
      ++fail_count;
      if (bytes.size() != last_bytes || fail_count <= 3 || (fail_count % 64) == 0) {
        last_bytes = bytes.size();
        std::fprintf(stderr,
                     "thumtoo: DEBUG_OVERLAY decode failed (bytes=%zu, n=%d)\n",
                     bytes.size(), fail_count);
      }
    }
    return false;
  }
  VipsImage* rgb = nullptr;
  if (vips_colourspace(img, &rgb, VIPS_INTERPRETATION_sRGB, nullptr) != 0 || !rgb) {
    g_object_unref(img);
    return false;
  }
  g_object_unref(img);
  if (vips_image_get_bands(rgb) != 3) {
    VipsImage* flat = nullptr;
    if (vips_extract_band(rgb, &flat, 0, "n", 3, nullptr) != 0 || !flat) {
      g_object_unref(rgb);
      return false;
    }
    g_object_unref(rgb);
    rgb = flat;
  }
  w = vips_image_get_width(rgb);
  h = vips_image_get_height(rgb);
  if (w <= 0 || h <= 0) {
    g_object_unref(rgb);
    return false;
  }
  size_t len = 0;
  void* mem = vips_image_write_to_memory(rgb, &len);
  g_object_unref(rgb);
  if (!mem || len == 0) {
    if (mem) g_free(mem);
    return false;
  }
  out_rgb.assign(static_cast<std::uint8_t*>(mem),
                 static_cast<std::uint8_t*>(mem) + len);
  g_free(mem);
  const std::size_t need =
      static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 3u;
  return out_rgb.size() >= need;
}

bool encode_jpeg(const std::uint8_t* rgb, int w, int h,
                 std::vector<std::uint8_t>& out, int quality = 85) {
  image_library_init();
  VipsImage* img = vips_image_new_from_memory(
      rgb, static_cast<size_t>(w) * static_cast<size_t>(h) * 3u, w, h, 3,
      VIPS_FORMAT_UCHAR);
  if (!img) {
    return false;
  }
  void* buf = nullptr;
  size_t len = 0;
  if (vips_jpegsave_buffer(img, &buf, &len, "Q", quality, nullptr) != 0 ||
      !buf) {
    g_object_unref(img);
    return false;
  }
  out.assign(static_cast<std::uint8_t*>(buf),
             static_cast<std::uint8_t*>(buf) + len);
  g_free(buf);
  g_object_unref(img);
  return true;
}

/** Raw rgb888 / rgba8 tiles (live PDF/image cells) — no VIPS decode. */
bool rgb_from_raw_codec(const std::string& codec, int width, int height,
                        const std::vector<std::uint8_t>& bytes, int& w, int& h,
                        std::vector<std::uint8_t>& out_rgb) {
  if (width < 1 || height < 1 || bytes.empty()) {
    return false;
  }
  const bool is_rgb =
      (codec == kTileCodecRgb888 || codec == "rgb888" || codec.empty());
  const bool is_rgba = (codec == "rgba8" || codec == "rgba8888");
  if (is_rgb) {
    const std::size_t need =
        static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 3u;
    if (bytes.size() < need) {
      return false;
    }
    w = width;
    h = height;
    out_rgb.assign(bytes.begin(),
                    bytes.begin() + static_cast<std::ptrdiff_t>(need));
    return true;
  }
  if (is_rgba) {
    const std::size_t need =
        static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u;
    if (bytes.size() < need) {
      return false;
    }
    w = width;
    h = height;
    out_rgb.resize(static_cast<std::size_t>(width) *
                    static_cast<std::size_t>(height) * 3u);
    for (int i = 0; i < width * height; ++i) {
      out_rgb[static_cast<std::size_t>(i * 3 + 0)] =
          bytes[static_cast<std::size_t>(i * 4 + 0)];
      out_rgb[static_cast<std::size_t>(i * 3 + 1)] =
          bytes[static_cast<std::size_t>(i * 4 + 1)];
      out_rgb[static_cast<std::size_t>(i * 3 + 2)] =
          bytes[static_cast<std::size_t>(i * 4 + 2)];
    }
    return true;
  }
  return false;
}

}  // namespace

bool debug_overlay_enabled() {
  return env_flag_on("THUMTOO_DEBUG_OVERLAY") ||
         env_flag_on("BILTOO_DEBUG_OVERLAY");
}

void debug_overlay_rgb888(std::uint8_t* rgb, int width, int height,
                          const std::vector<std::string>& lines) {
  if (!rgb || width < 8 || height < 8 || lines.empty()) {
    return;
  }
  const int border = std::max(2, std::min(width, height) / 64);
  draw_debug_border(rgb, width, height, border);

  // Glyph scale from sample size so text stays readable and covers most of
  // the tile/soft cell (hosts orient the bitmap — stamp is source-space).
  // 8×12 base glyphs read cleanly; nearest-neighbour scale fills the cell.
  int longest = 1;
  for (const auto& line : lines) {
    longest = std::max(longest, static_cast<int>(line.size()));
  }
  const int nlines = static_cast<int>(lines.size());
  // Fit block to ~80% of the shorter side.
  const int fit = std::max(8, std::min(width, height) * 4 / 5);
  const int scale_w = fit / std::max(1, longest * (kGw + kGGap));
  const int scale_h = fit / std::max(1, nlines * (kGh + 2));
  // Cap scale so large tiles do not turn three lines into a solid yellow slab.
  const int scale = std::max(1, std::min(16, std::min(scale_w, scale_h)));

  const int line_h = (kGh + 2) * scale;
  const int block_h = line_h * nlines;
  // Single centred block — each line centred independently (widths differ).
  const int y0 = std::max(border, (height - block_h) / 2);
  int yy = y0;
  for (const auto& line : lines) {
    const int lw = static_cast<int>(line.size()) * (kGw + kGGap) * scale;
    const int x = std::max(border, (width - lw) / 2);
    draw_text_line(rgb, width, height, x, yy, line, scale);
    yy += line_h;
  }
}

void debug_overlay_pixel_level(PixelLevel& px, std::string_view uri_tail,
                               int request_edge) {
  if (!debug_overlay_enabled() || px.bytes.empty()) {
    return;
  }
  static bool once = false;
  if (!once) {
    once = true;
    std::fprintf(stderr,
                 "thumtoo: DEBUG_OVERLAY active (stamping soft levels + tiles)\n");
  }
  int w = 0;
  int h = 0;
  std::vector<std::uint8_t> rgb;
  if (!rgb_from_raw_codec(px.codec, px.width, px.height, px.bytes, w, h, rgb) &&
      !rgb_from_encoded(px.bytes, w, h, rgb)) {
    return;
  }
  // Soft ladder only — not grid TILE. No filename / pixel size (unreadable
  // on small samples). Hosts orient the bitmap; stamp stays source-aligned.
  (void)uri_tail;
  (void)request_edge;
  std::vector<std::string> lines;
  lines.push_back("SOFT");
  lines.push_back("le=" + std::to_string(px.max_edge));
  debug_overlay_rgb888(rgb.data(), w, h, lines);

  std::vector<std::uint8_t> out;
  if (!encode_jpeg(rgb.data(), w, h, out, 90)) {
    return;
  }
  px.bytes = std::move(out);
  px.codec = "jpeg";
  px.width = w;
  px.height = h;
  std::fprintf(stderr,
               "thumtoo: DEBUG_OVERLAY soft %dx%d req=%d %s\n", w, h,
               request_edge, std::string(basename_tail(uri_tail)).c_str());
}

void debug_overlay_tile(TileBlob& tile, std::string_view uri_tail) {
  if (!debug_overlay_enabled() || tile.bytes.empty()) {
    return;
  }
  int w = 0;
  int h = 0;
  std::vector<std::uint8_t> rgb;
  // Live cells are rgb888 (e.g. 256×256×3 = 196608) — not a container format.
  // rgb_from_encoded would fail every time and spam the host.
  if (!rgb_from_raw_codec(tile.codec, tile.width, tile.height, tile.bytes, w, h,
                          rgb) &&
      !rgb_from_encoded(tile.bytes, w, h, rgb)) {
    return;
  }
  // Grid tile in source space — host orient/flip rotates the stamp with the
  // patch. Label: TILE / s=N / x,y only (no filename or pixel size).
  (void)uri_tail;
  {
    std::vector<std::string> lines;
    lines.push_back("TILE");
    lines.push_back("s=" + std::to_string(tile.scale));
    lines.push_back(std::to_string(tile.x) + "," + std::to_string(tile.y));
    debug_overlay_rgb888(rgb.data(), w, h, lines);
  }

  std::vector<std::uint8_t> out;
  if (!encode_jpeg(rgb.data(), w, h, out, 90)) {
    return;
  }
  tile.bytes = std::move(out);
  tile.codec = "jpeg";
  tile.width = w;
  tile.height = h;
}

}  // namespace thumtoo
