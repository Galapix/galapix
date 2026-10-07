// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "thumtoo/client.hpp"
#include "thumtoo/version.hpp"
#include "thumtoo/constants.hpp"
#include "thumtoo/image.hpp"
#include "thumtoo/pdf.hpp"
#include "thumtoo/uri.hpp"

#include <vips/vips.h>

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
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
      << "Fetch one Galapix-style tile for URI and write it as PNG (not JPEG)\n"
      << "so compression artifacts can be separated from low-res raster content.\n"
      << "\n"
      << "URI examples:\n"
      << "  file:///abs/photo.jpg\n"
      << "  file:///abs/doc.pdf//page:1\n"
      << "\n"
      << "Options:\n"
      << "  -h, --help           show this help\n"
      << "      --cache DIR      cache root (default: ~/.cache/thumtoo)\n"
      << "      --scale N        tile scale (default: 0; negative = denser for PDF)\n"
      << "      --x N --y N      tile indices (default: 0 0)\n"
      << "      --force          drop cached cell and regenerate via Client\n"
      << "      --cache-only     do not generate; fail if missing\n"
      << "      --raw-pdf        PDF only: region-rasterize cell to PNG, skip JPEG\n"
      << "                      encode path entirely (true source pixels)\n"
      << "  -o, --out PATH      output PNG (default: tile-s{N}-x{X}-y{Y}.png)\n"
      << "      --info          print size, coverage, theoretical dpi (PDF)\n"
      << "\n"
      << "By default the durable tile is loaded (or generated through the same\n"
      << "Client path Galapix uses), decoded, and saved as PNG.\n";
}

bool write_rgb_png(const std::filesystem::path& path, const std::uint8_t* rgb,
                   int width, int height) {
  if (!rgb || width <= 0 || height <= 0) return false;
  thumtoo::image_library_init();
  VipsImage* img = vips_image_new_from_memory_copy(
      rgb, static_cast<size_t>(width) * static_cast<size_t>(height) * 3u,
      width, height, 3, VIPS_FORMAT_UCHAR);
  if (!img) return false;
  const int rc = vips_image_write_to_file(img, path.string().c_str(), nullptr);
  g_object_unref(img);
  return rc == 0;
}

bool jpeg_bytes_to_png(const std::vector<std::uint8_t>& jpeg,
                       const std::filesystem::path& path, int* out_w,
                       int* out_h) {
  thumtoo::image_library_init();
  VipsImage* img = vips_image_new_from_buffer(
      jpeg.data(), jpeg.size(), "", nullptr);
  if (!img) return false;
  if (out_w) *out_w = vips_image_get_width(img);
  if (out_h) *out_h = vips_image_get_height(img);
  const int rc = vips_image_write_to_file(img, path.string().c_str(), nullptr);
  g_object_unref(img);
  return rc == 0;
}

std::string normalize_uri(std::string u) {
  if (u.empty()) return u;
  if (u.starts_with("file:") || u.starts_with("http:") ||
      u.starts_with("https:") || u.starts_with("content:")) {
    return u;
  }
  // Bare path → absolute file URI (keep //page: suffix if present).
  std::string path_part = u;
  std::string suffix;
  const auto pipe = u.find("//");
  if (pipe != std::string::npos && pipe > 0) {
    // "doc.pdf//page:1" without scheme
    path_part = u.substr(0, pipe);
    suffix = u.substr(pipe);
  }
  std::error_code ec;
  auto abs = std::filesystem::absolute(path_part, ec);
  if (ec) return u;
  return "file://" + abs.string() + suffix;
}

}  // namespace

int main(int argc, char** argv) {
  std::filesystem::path cache = default_cache_root();
  std::string uri;
  int scale = 0;
  int tile_x = 0;
  int tile_y = 0;
  bool force = false;
  bool cache_only = false;
  bool raw_pdf = false;
  bool show_info = false;
  std::filesystem::path out;

  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    if (a == "--version" || a == "-V") {
      thumtoo::print_version(std::cout);
      return 0;
    }
    auto need = [&](const char* name) -> std::string {
      if (i + 1 >= argc) {
        std::cerr << "missing value for " << name << "\n";
        std::exit(2);
      }
      return argv[++i];
    };
    if (a == "-h" || a == "--help") {
      usage(argv[0]);
      return 0;
    }
    if (a == "--cache") {
      cache = need("--cache");
      continue;
    }
    if (a == "--scale") {
      scale = std::stoi(need("--scale"));
      continue;
    }
    if (a == "--x") {
      tile_x = std::stoi(need("--x"));
      continue;
    }
    if (a == "--y") {
      tile_y = std::stoi(need("--y"));
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
    if (a == "--info") {
      show_info = true;
      continue;
    }
    if (a == "-o" || a == "--out") {
      out = need("--out");
      continue;
    }
    if (a.starts_with("-")) {
      std::cerr << "unknown option: " << a << "\n";
      usage(argv[0]);
      return 2;
    }
    if (!uri.empty()) {
      std::cerr << "unexpected argument: " << a << "\n";
      return 2;
    }
    uri = a;
  }

  if (uri.empty()) {
    usage(argv[0]);
    return 2;
  }
  uri = normalize_uri(uri);
  if (out.empty()) {
    out = std::filesystem::path(
        "tile-s" + std::to_string(scale) + "-x" + std::to_string(tile_x) +
        "-y" + std::to_string(tile_y) + ".png");
  }

  auto client = thumtoo::Client::open(cache);
  if (!client) {
    std::cerr << "failed to open cache: " << cache << "\n";
    return 1;
  }

  // Ensure size probe so meta exists (needed for cache keys / PDF layout).
  {
    bool done = false;
    client->request_size(uri, [&](std::string, thumtoo::SizeReply) {
      done = true;
    });
    client->drain();
    if (!done) {
      std::cerr << "size probe did not complete for " << uri << "\n";
    }
  }

  auto sz = client->get_size(uri);
  auto cov = client->get_tile_coverage(uri);

  if (show_info || true) {
    std::cout << "uri: " << uri << "\n";
    std::cout << "cache: " << cache << "\n";
    if (sz) {
      std::cout << "size: " << sz->width << "x" << sz->height << "\n";
    } else {
      std::cout << "size: (unknown)\n";
    }
    if (cov) {
      std::cout << "coverage: scale [" << cov->min_scale << " .. "
                << cov->max_scale << "]\n";
    } else {
      std::cout << "coverage: (none)\n";
    }
    std::cout << "request: scale=" << scale << " x=" << tile_x
              << " y=" << tile_y << "\n";
    if (thumtoo::is_pdf_page_uri(uri)) {
      if (scale < thumtoo::kPdfMinDurableTileScale) {
        std::cout << "note: scale " << scale
                  << " is finer than durable floor "
                  << thumtoo::kPdfMinDurableTileScale
                  << " — live-only (not written to cache)\n";
      }
      std::cout << "pdf dpi for scale: " << thumtoo::pdf_dpi_for_scale(scale)
                << " (layout_dpi=" << thumtoo::kPdfLayoutDpi << ")\n";
      if (sz) {
        auto full =
            thumtoo::pdf_page_size_at_scale(*sz, scale);
        std::cout << "full grid at scale: " << full.width << "x" << full.height
                  << "\n";
        std::cout << "tile origin px: (" << tile_x * thumtoo::kTileSize << ","
                  << tile_y * thumtoo::kTileSize << ")\n";
      }
    }
  }

  // --- Raw PDF path: no JPEG at all ---
  if (raw_pdf) {
    auto parsed = thumtoo::parse_pdf_uri(uri);
    if (!parsed) {
      std::cerr << "--raw-pdf requires a //page:N PDF URI\n";
      return 1;
    }
    if (!sz) {
      std::cerr << "no size for PDF page\n";
      return 1;
    }
    const thumtoo::Size full =
        thumtoo::pdf_page_size_at_scale(*sz, scale);
    const int left = tile_x * thumtoo::kTileSize;
    const int top = tile_y * thumtoo::kTileSize;
    if (left >= full.width || top >= full.height) {
      std::cerr << "tile origin outside page grid\n";
      return 1;
    }
    const int tw = std::min(thumtoo::kTileSize, full.width - left);
    const int th = std::min(thumtoo::kTileSize, full.height - top);
    const double dpi = thumtoo::pdf_dpi_for_scale(scale);
    std::cout << "raw-pdf: dpi=" << dpi << " crop=" << left << "," << top
              << " " << tw << "x" << th << "\n";
    auto raster = thumtoo::pdf_rasterize_page_region(
        parsed->pdf_path, parsed->page, dpi, left, top, tw, th);
    if (!raster || raster->rgb.empty()) {
      // Fallback: full page + soft crop (same as library).
      auto cell = thumtoo::pdf_build_tile_cell(
          parsed->pdf_path, parsed->page, scale, tile_x, tile_y);
      if (!cell || cell->bytes.empty()) {
        std::cerr << "raw-pdf rasterize failed\n";
        return 1;
      }
      // pdf_build_tile_cell returns JPEG — decode to PNG.
      int w = 0, h = 0;
      if (!jpeg_bytes_to_png(cell->bytes, out, &w, &h)) {
        std::cerr << "failed to write PNG from fallback JPEG cell\n";
        return 1;
      }
      std::cout << "wrote " << out << " (" << w << "x" << h
                << ") via pdf_build_tile_cell JPEG decode\n";
      std::cout << "note: fallback used JPEG encode path; prefer fixing region "
                   "render\n";
      return 0;
    }
    if (!write_rgb_png(out, raster->rgb.data(), raster->width,
                       raster->height)) {
      std::cerr << "failed to write PNG\n";
      return 1;
    }
    std::cout << "wrote " << out << " (" << raster->width << "x"
              << raster->height << ") raw RGB (no JPEG)\n";
    return 0;
  }

  // --- Client / cache path (same as Galapix) ---
  const bool had = client->has_tile(uri, scale, tile_x, tile_y);
  std::cout << "cache hit before: " << (had ? "yes" : "no") << "\n";

  if (force && had) {
    client->invalidate_tile(uri, scale, tile_x, tile_y);
    std::cout << "invalidated cached cell\n";
  }

  std::optional<thumtoo::TileBlob> blob;
  if (cache_only) {
    blob = client->get_tile(uri, scale, tile_x, tile_y);
    if (!blob) {
      std::cerr << "not in cache (--cache-only)\n";
      return 1;
    }
    std::cout << "source: cache\n";
  } else {
    bool done = false;
    client->request_tile(
        uri, scale, tile_x, tile_y,
        [&](std::string, int, int, int, std::optional<thumtoo::TileBlob> tb) {
          blob = std::move(tb);
          done = true;
        });
    client->drain();
    if (!done) {
      std::cerr << "request_tile did not complete\n";
      return 1;
    }
    if (!blob || blob->bytes.empty()) {
      std::cerr << "request_tile returned empty\n";
      return 1;
    }
    const bool now_cached = client->has_tile(uri, scale, tile_x, tile_y);
    if (had && !force) {
      std::cout << "source: durable cache\n";
    } else if (now_cached) {
      std::cout << "source: generated and stored (durable)\n";
    } else {
      std::cout << "source: generated live-only (not stored";
      if (thumtoo::is_pdf_page_uri(uri) &&
          scale < thumtoo::kPdfMinDurableTileScale) {
        std::cout << "; scale < kPdfMinDurableTileScale="
                  << thumtoo::kPdfMinDurableTileScale;
      }
      std::cout << ")\n";
    }
  }

  std::cout << "blob: " << blob->bytes.size() << " bytes codec=" << blob->codec
            << " meta=" << blob->width << "x" << blob->height << "\n";

  int w = 0, h = 0;
  if (blob->codec == thumtoo::kTileCodecRgb888 || blob->codec == "rgb888") {
    w = blob->width;
    h = blob->height;
    if (!write_rgb_png(out, blob->bytes.data(), w, h)) {
      std::cerr << "failed to write rgb888 → PNG\n";
      return 1;
    }
    std::cout << "wrote " << out << " (" << w << "x" << h
              << ") PNG from raw rgb888 (no JPEG)\n";
  } else {
    if (!jpeg_bytes_to_png(blob->bytes, out, &w, &h)) {
      std::cerr << "failed to decode tile JPEG → PNG (codec=" << blob->codec
                << ")\n";
      return 1;
    }
    std::cout << "wrote " << out << " (" << w << "x" << h
              << ") PNG from tile JPEG decode\n";
  }
  if (w < 200 && h < 200 && scale <= 0) {
    std::cout << "note: small decoded size at fine scale may mean the stored "
                 "cell is low-res content labeled as scale "
              << scale << "\n";
  }
  return 0;
}
