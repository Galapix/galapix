// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "thumtoo/client.hpp"
#include "thumtoo/layout.hpp"
#include "thumtoo/constants.hpp"
#include "thumtoo/image.hpp"
#include "thumtoo/uri.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {
int g_failures = 0;
void expect(bool c, const char* m) {
  if (!c) {
    std::cerr << "FAIL: " << m << "\n";
    ++g_failures;
  }
}

// Minimal 2x2 RGB24 BMP (raw BMP written without image deps in the test TU).
void write_tiny_bmp(const fs::path& path, int w, int h) {
  const int row_raw = w * 3;
  const int row_pad = (4 - (row_raw % 4)) % 4;
  const int row_stride = row_raw + row_pad;
  const int pixel_size = row_stride * h;
  const int file_size = 14 + 40 + pixel_size;
  std::vector<unsigned char> buf(static_cast<size_t>(file_size), 0);
  auto put16 = [&](int off, int v) {
    buf[static_cast<size_t>(off)] = static_cast<unsigned char>(v & 0xff);
    buf[static_cast<size_t>(off + 1)] = static_cast<unsigned char>((v >> 8) & 0xff);
  };
  auto put32 = [&](int off, int v) {
    put16(off, v & 0xffff);
    put16(off + 2, (v >> 16) & 0xffff);
  };
  buf[0] = 'B';
  buf[1] = 'M';
  put32(2, file_size);
  put32(10, 54);
  put32(14, 40);
  put32(18, w);
  put32(22, h);
  put16(26, 1);
  put16(28, 24);
  put32(34, pixel_size);
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      const int i = 54 + y * row_stride + x * 3;
      buf[static_cast<size_t>(i) + 0] = static_cast<unsigned char>(x * 40);
      buf[static_cast<size_t>(i) + 1] = static_cast<unsigned char>(y * 40);
      buf[static_cast<size_t>(i) + 2] = 200;
    }
  }
  std::ofstream out(path, std::ios::binary);
  out.write(reinterpret_cast<const char*>(buf.data()),
            static_cast<std::streamsize>(buf.size()));
}
}  // namespace

int main() {
  using namespace thumtoo;
  const auto root =
      fs::temp_directory_path() / "thumtoo-client-test" /
      std::to_string(
          std::chrono::steady_clock::now().time_since_epoch().count());
  fs::create_directories(root);

  constexpr int W = 320;
  constexpr int H = 200;
  const auto img = root / "gradient.bmp";
  write_tiny_bmp(img, W, H);

  const auto uri = file_uri_from_path(img);
  const auto cache = root / "cache";

  {
    image_library_init();
    auto client = Client::open(cache, {}, 0, cache);
    expect(!client->get_size(uri).has_value(), "no size before request");

    // --- probe ---
    bool called = false;
    std::optional<Size> got;
    client->request_size(uri, [&](std::string, SizeReply r) {
      called = true;
      got = r.size;
    });
    client->drain();
    expect(called, "size callback invoked");
    expect(got.has_value(), "size present");
    expect(got && got->width == W && got->height == H, "native size");

    auto meta = client->get_meta(uri);
    expect(meta.has_value(), "meta after probe");
    expect(meta && meta->status == ContentStatus::Incomplete,
           "Incomplete after probe (no tiles yet)");
    expect(meta && meta->content_id.find("sha256:") == 0, "sha256 content id");
    expect(!client->get_pixels(uri, 256).has_value(),
           "no durable pixels until tiles");

    // --- session soft pixels (tiles-first: not durable) ---
    bool called_px = false;
    client->request_pixels(uri, 128, [&](std::string, int, std::optional<PixelLevel> p) {
      called_px = true;
      expect(p.has_value(), "request_pixels data");
      expect(p && !p->bytes.empty(), "pixel bytes");
      expect(p && p->codec == "jxl", "codec jxl");
      expect(p && p->max_edge <= 128, "request edge");
    });
    client->drain();
    expect(called_px, "request_pixels callback");
    meta = client->get_meta(uri);
    expect(meta && meta->status == ContentStatus::Incomplete,
           "still Incomplete after session soft (no tiles)");
    expect(!client->get_pixels(uri, 256).has_value(),
           "no durable soft under tiles-first");
    expect(!fs::exists(cache / "legacy"), "no legacy/ directory");

    // --- durable grid tiles ---
    expect(!client->has_tile(uri, 0, 0, 0), "has_tile false before request");
    bool called_tile = false;
    client->request_tile(uri, 0, 0, 0,
                         [&](std::string, int scale, int x, int y,
                             std::optional<TileBlob> tb) {
                           called_tile = true;
                           expect(scale == 0 && x == 0 && y == 0, "tile coords");
                           expect(tb.has_value(), "request_tile data");
                           expect(tb && !tb->bytes.empty(), "tile bytes");
                           expect(tb && tb->codec == "rgb888", "tile codec rgb888");
                           expect(tb && tb->width > 0 && tb->height > 0, "tile dims");
                         });
    client->drain();
    expect(called_tile, "request_tile callback");
    expect(client->has_tile(uri, 0, 0, 0), "has_tile after request");
    auto t0 = client->get_tile(uri, 0, 0, 0);
    expect(t0.has_value(), "get_tile cache hit");
    expect(t0 && t0->codec == "jpeg", "durable tile codec jpeg");

    meta = client->get_meta(uri);
    expect(meta && meta->status == ContentStatus::Ready,
           "Ready after durable tiles");

    auto cov = client->get_tile_coverage(uri);
    expect(cov.has_value(), "tile coverage");
    expect(cov && cov->size.width == W && cov->size.height == H, "coverage size");
    expect(cov && cov->max_scale >= cov->min_scale, "coverage scales");

    // --- source bytes by file URI and by content-id ---
    auto bytes = client->read_source_bytes(uri);
    expect(bytes.has_value() && !bytes->empty(), "read_source_bytes file");
    auto cid = client->resolve_content_id(uri);
    expect(cid.has_value(), "resolve_content_id after probe");
    expect(cid && cid->find("sha256:") == 0, "content-id prefix");
    if (cid) {
      auto locs = client->list_uris_for_content_id(*cid);
      expect(!locs.empty(), "list_uris_for_content_id non-empty");
      if (!locs.empty()) {
        expect(locs.front().uri == uri || locs.front().uri.find("file:") == 0,
               "locator uri is file");
      }
      auto by_id = client->read_source_bytes(*cid);
      expect(by_id.has_value() && by_id->size() == bytes->size(),
             "read_source_bytes via content-id");
    }
  }

  std::error_code ec;
  fs::remove_all(root, ec);
  if (g_failures) {
    std::cerr << g_failures << " failure(s)\n";
    return 1;
  }
  std::cout << "ok\n";
  return 0;
}
