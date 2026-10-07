// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "thumtoo/lqip.hpp"

#include <cmath>
#include <iostream>
#include <vector>

namespace {

int g_failures = 0;

void expect(bool cond, const char* msg) {
  if (!cond) {
    std::cerr << "FAIL: " << msg << "\n";
    ++g_failures;
  }
}

}  // namespace

int main() {
  // Solid red 32x32 RGBA
  constexpr int W = 32;
  constexpr int H = 32;
  std::vector<std::uint8_t> rgba(static_cast<std::size_t>(W * H * 4));
  for (int i = 0; i < W * H; ++i) {
    rgba[static_cast<std::size_t>(i * 4 + 0)] = 220;
    rgba[static_cast<std::size_t>(i * 4 + 1)] = 40;
    rgba[static_cast<std::size_t>(i * 4 + 2)] = 40;
    rgba[static_cast<std::size_t>(i * 4 + 3)] = 255;
  }

  auto hash = thumtoo::thumbhash_encode_rgba(W, H, rgba);
  expect(!hash.empty(), "hash non-empty");
  expect(hash.size() >= 5 && hash.size() <= 40, "hash size in ThumbHash range");

  auto img = thumtoo::thumbhash_decode_rgba(hash);
  expect(img.has_value(), "decode ok");
  if (img) {
    expect(img->width > 0 && img->height > 0, "decode dims");
    expect(img->rgba.size() ==
               static_cast<std::size_t>(img->width * img->height * 4),
           "rgba size");
    // Average should stay reddish
    double ar = 0, ag = 0, ab = 0;
    const int n = img->width * img->height;
    for (int i = 0; i < n; ++i) {
      ar += img->rgba[static_cast<std::size_t>(i * 4 + 0)];
      ag += img->rgba[static_cast<std::size_t>(i * 4 + 1)];
      ab += img->rgba[static_cast<std::size_t>(i * 4 + 2)];
    }
    ar /= n;
    ag /= n;
    ab /= n;
    expect(ar > ag && ar > ab, "decoded average still red-dominant");
  }

  // RGB888 helper
  std::vector<std::uint8_t> rgb(static_cast<std::size_t>(W * H * 3));
  for (int i = 0; i < W * H; ++i) {
    rgb[static_cast<std::size_t>(i * 3 + 0)] = 40;
    rgb[static_cast<std::size_t>(i * 3 + 1)] = 40;
    rgb[static_cast<std::size_t>(i * 3 + 2)] = 200;
  }
  auto hash2 = thumtoo::thumbhash_encode_rgb888(rgb.data(), W, H, 32);
  expect(!hash2.empty(), "rgb888 hash");
  auto img2 = thumtoo::thumbhash_decode_rgba(hash2);
  expect(img2.has_value(), "rgb888 decode");

  if (g_failures) {
    std::cerr << g_failures << " failure(s)\n";
    return 1;
  }
  return 0;
}
