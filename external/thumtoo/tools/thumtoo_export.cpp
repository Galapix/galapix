// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/// Assemble all exclusive tiles for a URI at one pyramid scale into a PNG.
/// Debugging aid for tile seams / missing lines (host paint vs encode).

#include "thumtoo/client.hpp"
#include "thumtoo/constants.hpp"
#include "thumtoo/image.hpp"
#include "thumtoo/pdf.hpp"
#include "thumtoo/uri.hpp"
#include "thumtoo/version.hpp"

#include <vips/vips.h>

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace {

std::filesystem::path default_cache_root() {
  if (const char* xdg = std::getenv("XDG_CACHE_HOME"); xdg && *xdg) {
    return std::filesystem::path(xdg) / "thumtoo";
  }
  if (const char* home = std::getenv("HOME"); home && *home) {
    return std::filesystem::path(home) / ".cache" / "thumtoo";
  }
  return std::filesystem::path(".cache") / "thumtoo";
}

void usage(const char* argv0) {
  std::cerr
      << "Usage: " << argv0 << " [OPTION]... URI\n"
      << "\n"
      << "Assemble every exclusive tile at one pyramid scale into a single PNG.\n"
      << "Same grid math as Galapix/thumtoo (256 step, dim_at_tile_scale).\n"
      << "\n"
      << "URI examples:\n"
      << "  file:///abs/photo.jpg\n"
      << "  file:///abs/doc.pdf//page:1\n"
      << "  /abs/doc.pdf//page:1\n"
      << "\n"
      << "Options:\n"
      << "  -h, --help           show this help\n"
      << "      --cache DIR      cache root (default: ~/.cache/thumtoo)\n"
      << "      --scale N        pyramid scale (default: 0 = native level)\n"
      << "      --force          regenerate missing cells via Client\n"
      << "      --cache-only     fail if any cell is missing (no generate)\n"
      << "      --raw-pdf        PDF only: region-rasterize each cell (no JPEG)\n"
      << "  -o, --out PATH      output PNG (default: export-s{N}.png)\n"
      << "\n"
      << "Output size is level size at --scale (floor-half of native), not\n"
      << "necessarily native width×height when scale > 0.\n";
}

bool write_rgb_png(const std::filesystem::path& path, const std::uint8_t* rgb,
                   int width, int height) {
  if (!rgb || width <= 0 || height <= 0) return false;
  thumtoo::image_library_init();
  VipsImage* img = vips_image_new_from_memory_copy(
      rgb, static_cast<size_t>(width) * static_cast<size_t>(height) * 3u, width,
      height, 3, VIPS_FORMAT_UCHAR);
  if (!img) return false;
  img->Type = VIPS_INTERPRETATION_sRGB;
  const int rc = vips_image_write_to_file(img, path.string().c_str(), nullptr);
  g_object_unref(img);
  return rc == 0;
}

/// Decode JPEG tile bytes → RGB888 (owns returned buffer via vector).
bool jpeg_to_rgb(const std::vector<std::uint8_t>& jpeg, int* out_w, int* out_h,
                 std::vector<std::uint8_t>* out_rgb) {
  if (!out_w || !out_h || !out_rgb || jpeg.empty()) return false;
  thumtoo::image_library_init();
  VipsImage* im = nullptr;
  if (vips_jpegload_buffer(const_cast<std::uint8_t*>(jpeg.data()),
                           static_cast<size_t>(jpeg.size()), &im, nullptr) != 0 ||
      !im) {
    return false;
  }
  if (vips_image_wio_input(im) != 0) {
    g_object_unref(im);
    return false;
  }
  VipsImage* rgb = im;
  if (im->Type != VIPS_INTERPRETATION_sRGB || im->Bands != 3 ||
      im->BandFmt != VIPS_FORMAT_UCHAR) {
    VipsImage* conv = nullptr;
    if (vips_colourspace(im, &conv, VIPS_INTERPRETATION_sRGB, nullptr) != 0 ||
        !conv) {
      g_object_unref(im);
      return false;
    }
    g_object_unref(im);
    im = conv;
    rgb = im;
    if (im->BandFmt != VIPS_FORMAT_UCHAR) {
      VipsImage* u8 = nullptr;
      if (vips_cast_uchar(im, &u8, nullptr) != 0 || !u8) {
        g_object_unref(im);
        return false;
      }
      g_object_unref(im);
      im = u8;
      rgb = im;
    }
    if (im->Bands > 3) {
      VipsImage* extr = nullptr;
      if (vips_extract_band(im, &extr, 0, "n", 3, nullptr) != 0 || !extr) {
        g_object_unref(im);
        return false;
      }
      g_object_unref(im);
      im = extr;
      rgb = im;
    }
  }
  const int w = vips_image_get_width(rgb);
  const int h = vips_image_get_height(rgb);
  if (w < 1 || h < 1) {
    g_object_unref(rgb);
    return false;
  }
  size_t len = 0;
  void* buf = vips_image_write_to_memory(rgb, &len);
  g_object_unref(rgb);
  if (!buf || len < static_cast<size_t>(w) * static_cast<size_t>(h) * 3u) {
    if (buf) g_free(buf);
    return false;
  }
  out_rgb->assign(static_cast<std::uint8_t*>(buf),
                  static_cast<std::uint8_t*>(buf) + len);
  g_free(buf);
  *out_w = w;
  *out_h = h;
  return true;
}

void blit_rgb(std::vector<std::uint8_t>& canvas, int sw, int sh, int dst_x,
              int dst_y, const std::uint8_t* src, int src_w, int src_h) {
  if (!src || src_w < 1 || src_h < 1) return;
  for (int y = 0; y < src_h; ++y) {
    const int cy = dst_y + y;
    if (cy < 0 || cy >= sh) continue;
    const int copy_w = std::min(src_w, sw - dst_x);
    if (copy_w <= 0 || dst_x >= sw) continue;
    std::memcpy(canvas.data() + (static_cast<size_t>(cy) * static_cast<size_t>(sw) +
                                 static_cast<size_t>(dst_x)) *
                                    3u,
                src + static_cast<size_t>(y) * static_cast<size_t>(src_w) * 3u,
                static_cast<size_t>(copy_w) * 3u);
  }
}

}  // namespace

int main(int argc, char** argv) {
  std::string uri;
  std::filesystem::path cache = default_cache_root();
  std::filesystem::path out;
  int scale = 0;
  bool force = false;
  bool cache_only = false;
  bool raw_pdf = false;

  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    if (a == "-h" || a == "--help") {
      usage(argv[0]);
      return 0;
    }
    if (a == "--cache" && i + 1 < argc) {
      cache = argv[++i];
      continue;
    }
    if (a == "--scale" && i + 1 < argc) {
      scale = std::atoi(argv[++i]);
      continue;
    }
    if (a == "--force") {
      force = true;
      continue;
    }
    if (a == "--cache-only") {
      cache_only = true;
      continue;
    }
    if (a == "--raw-pdf") {
      raw_pdf = true;
      continue;
    }
    if ((a == "-o" || a == "--out") && i + 1 < argc) {
      out = argv[++i];
      continue;
    }
    if (!a.empty() && a[0] == '-') {
      std::cerr << "unknown option: " << a << "\n";
      usage(argv[0]);
      return 2;
    }
    if (uri.empty()) {
      uri = a;
    } else {
      std::cerr << "unexpected argument: " << a << "\n";
      return 2;
    }
  }
  if (uri.empty()) {
    usage(argv[0]);
    return 2;
  }
  if (out.empty()) {
    out = "export-s" + std::to_string(scale) + ".png";
  } else {
    std::error_code ec;
    if (std::filesystem::is_directory(out, ec) ||
        (!ec && out.string().back() == '/')) {
      out /= ("export-s" + std::to_string(scale) + ".png");
    }
  }

  // Normalize path-like args to file URIs. Preserve //page: (and other)
  // pipe suffixes — lexically_normal() would collapse // to /.
  if (!uri.starts_with("file:") && !uri.starts_with("http:") &&
      !uri.starts_with("https:") && !uri.starts_with("content:")) {
    std::string path_part = uri;
    std::string suffix;
    const auto pipe = uri.find("//");
    if (pipe != std::string::npos && pipe > 0) {
      path_part = uri.substr(0, pipe);
      suffix = uri.substr(pipe);
    }
    std::error_code ec;
    auto abs = std::filesystem::absolute(path_part, ec);
    if (!ec) {
      uri = thumtoo::file_uri_from_path(abs.lexically_normal()) + suffix;
    }
  }

  auto client = thumtoo::Client::open(cache, {}, 0, cache);
  if (!client) {
    std::cerr << "failed to open cache at " << cache << "\n";
    return 1;
  }

  // Ensure size is known (probe if needed).
  if (!client->get_size(uri)) {
    bool done = false;
    client->request_size(uri, [&](std::string, thumtoo::SizeReply) { done = true; });
    client->drain();
    if (!done) {
      std::cerr << "size probe did not complete for " << uri << "\n";
      return 1;
    }
  }
  auto sz = client->get_size(uri);
  if (!sz || sz->width < 1 || sz->height < 1) {
    std::cerr << "no size for " << uri << "\n";
    return 1;
  }

  const int sw = thumtoo::dim_at_tile_scale(sz->width, scale);
  const int sh = thumtoo::dim_at_tile_scale(sz->height, scale);
  if (sw < 1 || sh < 1) {
    std::cerr << "level size is empty at scale " << scale << "\n";
    return 1;
  }
  const int nx = (sw + thumtoo::kTileSize - 1) / thumtoo::kTileSize;
  const int ny = (sh + thumtoo::kTileSize - 1) / thumtoo::kTileSize;

  std::cout << "uri: " << uri << "\n"
            << "cache: " << cache << "\n"
            << "native: " << sz->width << "x" << sz->height << "\n"
            << "scale: " << scale << " level: " << sw << "x" << sh
            << " tiles: " << nx << "x" << ny << "\n"
            << "out: " << out << "\n";

  std::vector<std::uint8_t> canvas(
      static_cast<size_t>(sw) * static_cast<size_t>(sh) * 3u, 255);

  // --- Raw PDF: no JPEG ---
  if (raw_pdf) {
    auto parsed = thumtoo::parse_pdf_uri(uri);
    if (!parsed) {
      std::cerr << "--raw-pdf requires a //page:N PDF URI\n";
      return 1;
    }
    for (int ty = 0; ty < ny; ++ty) {
      for (int tx = 0; tx < nx; ++tx) {
        int left = 0, top = 0, tw = 0, th = 0;
        thumtoo::tile_cell_pixel_rect(sw, sh, tx, ty, &left, &top, &tw, &th);
        if (tw < 1 || th < 1) continue;
        // Same path as durable/live tiles.
        auto cell = thumtoo::pdf_render_tile_cell(parsed->pdf_path,
                                                 parsed->page, scale, tx, ty);
        if (!cell.raster || cell.raster->rgb.empty()) {
          std::cerr << "raw-pdf cell " << tx << "," << ty << " "
                    << thumtoo::tile_status_name(cell.status) << ": "
                    << cell.error << "\n";
          return 1;
        }
        auto& raster = cell.raster;
        if (raster->width != tw || raster->height != th) {
          std::cerr << "warn: cell " << tx << "," << ty << " raster "
                    << raster->width << "x" << raster->height << " expected "
                    << tw << "x" << th << "\n";
        }
        blit_rgb(canvas, sw, sh, left, top, raster->rgb.data(), raster->width,
                 raster->height);
      }
    }
    if (!write_rgb_png(out, canvas.data(), sw, sh)) {
      std::cerr << "failed to write PNG\n";
      return 1;
    }
    std::cout << "wrote " << out << " (" << sw << "x" << sh
              << ") raw-pdf assemble\n";
    return 0;
  }

  // --- Cache / Client tiles ---
  for (int ty = 0; ty < ny; ++ty) {
    for (int tx = 0; tx < nx; ++tx) {
      int left = 0, top = 0, tw = 0, th = 0;
      thumtoo::tile_cell_pixel_rect(sw, sh, tx, ty, &left, &top, &tw, &th);
      if (tw < 1 || th < 1) continue;

      // get_tile treats wrong-dimension Store rows as miss (stale layout).
      // One request_tile re-encodes; put_tile ON CONFLICT replaces the row.
      // No retry loop — second mismatch is fatal. invalidate_tile is a no-op.
      if (force && client->has_tile(uri, scale, tx, ty)) {
        // force: still request even when has_tile; worker short-circuits only
        // on get_tile hit (dimension-checked).
        (void)0;
      }

      auto tile = client->get_tile(uri, scale, tx, ty);
      if ((!tile || tile->bytes.empty()) && !cache_only) {
        bool done = false;
        bool ok = false;
        client->request_tile(
            uri, scale, tx, ty,
            [&](std::string, int, int, int, std::optional<thumtoo::TileBlob> tb) {
              done = true;
              ok = static_cast<bool>(tb);
            });
        client->drain();
        if (!done || !ok) {
          std::cerr << "request_tile failed at scale=" << scale << " x=" << tx
                    << " y=" << ty << "\n";
          return 1;
        }
        tile = client->get_tile(uri, scale, tx, ty);
      }
      if (!tile || tile->bytes.empty()) {
        std::cerr << "missing tile scale=" << scale << " x=" << tx << " y=" << ty
                  << "\n";
        return 1;
      }

      int bw = 0, bh = 0;
      std::vector<std::uint8_t> rgb;
      if (!jpeg_to_rgb(tile->bytes, &bw, &bh, &rgb)) {
        std::cerr << "JPEG decode failed at " << tx << "," << ty << "\n";
        return 1;
      }
      if (bw != tw || bh != th) {
        std::cerr << "error: cell " << tx << "," << ty << " decoded " << bw
                  << "x" << bh << " expected " << tw << "x" << th
                  << " after at most one encode (not retrying)\n";
        return 1;
      }
      blit_rgb(canvas, sw, sh, left, top, rgb.data(), bw, bh);
    }
  }

  if (!write_rgb_png(out, canvas.data(), sw, sh)) {
    std::cerr << "failed to write PNG\n";
    return 1;
  }
  std::cout << "wrote " << out << " (" << sw << "x" << sh << ") from " << (nx * ny)
            << " tiles\n";
  return 0;
}
