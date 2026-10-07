// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * thumtoo::lod::ClientTileBackend and decode_tile_bitmap: every requested cell
 * comes back exactly once with a FetchStatus, Ok cells as RGBA8 pixels.
 */

#include "thumtoo/client.hpp"
#include "thumtoo/image.hpp"
#include "thumtoo/lod/client_tile_backend.hpp"
#include "thumtoo/uri.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

namespace {

int g_failures = 0;
void expect(bool c, const std::string& m) {
  if (!c) {
    std::cerr << "FAIL: " << m << "\n";
    ++g_failures;
  }
}

void write_bmp(const fs::path& path, int w, int h) {
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
      buf[static_cast<size_t>(i) + 0] = static_cast<unsigned char>(x);
      buf[static_cast<size_t>(i) + 1] = static_cast<unsigned char>(y);
      buf[static_cast<size_t>(i) + 2] = 200;
    }
  }
  std::ofstream out(path, std::ios::binary);
  out.write(reinterpret_cast<const char*>(buf.data()),
            static_cast<std::streamsize>(buf.size()));
}

void test_decode() {
  using namespace thumtoo;
  using thumtoo::lod::decode_tile_bitmap;

  TileBlob rgb;
  rgb.width = 2;
  rgb.height = 1;
  rgb.codec = kTileCodecRgb888;
  rgb.bytes = {1, 2, 3, 4, 5, 6};
  auto a = decode_tile_bitmap(rgb);
  expect(a && a->codec == "rgba8" && a->width == 2 && a->height == 1,
         "rgb888 decodes to rgba8");
  expect(a && a->bytes == std::vector<std::uint8_t>{1, 2, 3, 255, 4, 5, 6, 255},
         "rgb888 expands with opaque alpha");

  TileBlob rgba;
  rgba.width = 1;
  rgba.height = 1;
  rgba.codec = "rgba8";
  rgba.bytes = {9, 8, 7, 6};
  auto b = decode_tile_bitmap(rgba);
  expect(b && b->bytes == std::vector<std::uint8_t>{9, 8, 7, 6}, "rgba8 passes through");

  std::vector<std::uint8_t> px(64 * 32 * 3, 100);
  auto jpeg = encode_tile_cell_rgb(px.data(), 64, 32, 0, 0, 0, 90);
  expect(jpeg && !jpeg->bytes.empty(), "encode jpeg cell");
  if (jpeg) {
    auto c = decode_tile_bitmap(*jpeg);
    expect(c && c->codec == "rgba8" && c->width == 64 && c->height == 32 &&
               c->bytes.size() == 64u * 32u * 4u,
           "jpeg decodes to 64x32 rgba8");
  }

  TileBlob bad;
  bad.width = 4;
  bad.height = 4;
  bad.codec = kTileCodecRgb888;
  bad.bytes = {1, 2, 3};
  expect(!decode_tile_bitmap(bad), "short rgb888 payload is rejected");
}

}  // namespace

int main() {
  using namespace thumtoo;
  using namespace thumtoo::lod;
  image_library_init();

  test_decode();

  const auto root =
      fs::temp_directory_path() / "thumtoo-lod-client-backend" /
      std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
  fs::create_directories(root);
  const auto img = root / "grid.bmp";
  write_bmp(img, 600, 400);  // scale 0: 3x2 cells, edge cells 88 / 144 px

  std::shared_ptr<Client> client = Client::open(root / "cache", {}, 2, root / "cache");
  expect(!!client, "open client");
  if (!client) {
    return 1;
  }

  ClientTileBackend backend(client, file_uri_from_path(img));

  std::mutex mu;
  std::map<std::uint64_t, std::vector<FetchResult>> results;
  std::vector<FetchRequest> cells{
      {{0, 0, 0}, 1}, {{0, 2, 1}, 2}, {{0, 9, 9}, 3}, {{1, 0, 0}, 4}};
  backend.fetch(cells, [&](FetchResult r) {
    std::lock_guard<std::mutex> lock(mu);
    results[r.ticket].push_back(std::move(r));
  });

  for (int i = 0; i < 200; ++i) {
    client->drain();
    std::lock_guard<std::mutex> lock(mu);
    if (results.size() == cells.size()) {
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  std::lock_guard<std::mutex> lock(mu);
  for (FetchRequest const& c : cells) {
    expect(results[c.ticket].size() == 1,
           "exactly one result for ticket " + std::to_string(c.ticket));
  }
  auto first = [&](std::uint64_t t) { return results[t].empty() ? FetchResult{} : results[t].front(); };

  FetchResult const r1 = first(1);
  expect(r1.status == FetchStatus::Ok && r1.key == TileKey{0, 0, 0}, "cell (0,0,0) Ok");
  expect(r1.bitmap.codec == "rgba8" && r1.bitmap.width == 256 && r1.bitmap.height == 256 &&
             r1.bitmap.bytes.size() == 256u * 256u * 4u,
         "cell (0,0,0) is 256x256 rgba8");

  FetchResult const r2 = first(2);
  expect(r2.status == FetchStatus::Ok && r2.bitmap.width == 88 && r2.bitmap.height == 144,
         "edge cell (0,2,1) is 88x144, got " + std::to_string(r2.bitmap.width) + "x" +
             std::to_string(r2.bitmap.height));

  expect(first(3).status == FetchStatus::Unavailable,
         std::string("out-of-grid cell is Unavailable, got ") + fetch_status_name(first(3).status));

  FetchResult const r4 = first(4);
  expect(r4.status == FetchStatus::Ok && r4.bitmap.width == 256 && r4.bitmap.height == 200,
         "scale 1 cell is 300x200 floor-halved → 256x200");

  fs::remove_all(root);

  if (g_failures) {
    std::cerr << g_failures << " failure(s)\n";
    return 1;
  }
  std::cout << "test_lod_client_backend: all passed\n";
  return 0;
}
