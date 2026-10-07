// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/// get_pixels_from_tiles: nullopt without pyramid; constants present.

#include "thumtoo/client.hpp"
#include "thumtoo/constants.hpp"
#include "thumtoo/types.hpp"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

namespace {

int g_failures = 0;

void expect(bool c, const char* m) {
  if (!c) {
    std::cerr << "FAIL: " << m << "\n";
    ++g_failures;
  }
}

}  // namespace

int main() {
  using namespace thumtoo;
  const fs::path root =
      fs::temp_directory_path() / "thumtoo-tile-synth-test" /
      std::to_string(
          std::chrono::steady_clock::now().time_since_epoch().count());
  fs::create_directories(root);
  const fs::path cache = root / "cache";

  auto client = Client::open(cache, {}, 0, cache);
  expect(static_cast<bool>(client), "client open");

  const std::string uri = "file:///nonexistent/does-not-exist.jpg";
  expect(!client->get_pixels_from_tiles(uri, 512).has_value(),
         "no tiles → nullopt");
  expect(!client->get_pixels(uri, 256).has_value(), "no soft → nullopt");

  expect(kBatchMaxEdge == 1024, "kBatchMaxEdge");
  expect(static_cast<int>(PixelSource::TileSynth) == 4, "TileSynth enum");

  // Floor-half scale dims (not ceil): 1000 → 500 → 250 → 125 → 62 …
  expect(dim_at_tile_scale(1000, 0) == 1000, "scale0 native");
  expect(dim_at_tile_scale(1000, 1) == 500, "scale1 floor");
  expect(dim_at_tile_scale(1000, 4) == 62, "scale4 floor not ceil 63");
  expect(dim_at_tile_scale(1000, 4) != ((1000 + 15) >> 4), "differs from ceil");

  client.reset();
  fs::remove_all(root);

  if (g_failures) {
    std::cerr << g_failures << " failure(s)\n";
    return 1;
  }
  std::cout << "ok\n";
  return 0;
}
