// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
//
// ThumbHash encode/decode adapted from Evan Wallace's reference
// (https://github.com/evanw/thumbhash, MIT).
// Handsum c3q4: Wuffs basic-handsum-decode + encode (Apache-2.0 OR MIT).

#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace thumtoo {

/// Kind stored in content.lqip_kind (schema v2+).
inline constexpr int kLqipKindNone = 0;
inline constexpr int kLqipKindThumbHash = 1;
inline constexpr int kLqipKindHandsum = 2;
/// Durable EXIF/container JPEG preview — not ThumbHash; never tile cells.
inline constexpr int kLqipKindEmbeddedJpeg = 3;

struct LqipRgba {
  int width = 0;
  int height = 0;
  std::vector<std::uint8_t> rgba;  // width*height*4, not premultiplied
};

/// Encode RGBA8 image (w,h ≤ 100) to ThumbHash bytes (~25–37).
[[nodiscard]] std::vector<std::uint8_t> thumbhash_encode_rgba(
    int w, int h, std::span<const std::uint8_t> rgba);

/// Decode ThumbHash to a small RGBA placeholder (typically ~32×32-ish).
[[nodiscard]] std::optional<LqipRgba> thumbhash_decode_rgba(
    std::span<const std::uint8_t> hash);

/// Downscale RGB888 (no alpha) to fit in max_edge, then ThumbHash-encode.
[[nodiscard]] std::vector<std::uint8_t> thumbhash_encode_rgb888(
    const std::uint8_t* rgb, int width, int height, int max_edge = 32);

/// Handsum c3q4 encode (always 147 bytes) from RGB888.
[[nodiscard]] std::vector<std::uint8_t> handsum_encode_rgb888(
    const std::uint8_t* rgb, int width, int height);

/// Handsum decode (c3q4 and compatible magic).
[[nodiscard]] std::optional<LqipRgba> handsum_decode(
    std::span<const std::uint8_t> bytes);

/// Decode LQIP blob: Handsum (magic) or ThumbHash.
[[nodiscard]] std::optional<LqipRgba> lqip_decode_rgba(
    std::span<const std::uint8_t> bytes);

}  // namespace thumtoo
