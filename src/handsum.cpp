// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Handsum c3q4 encode (147 bytes) + decode wrapper.
// Decode: Wuffs basic-handsum-decode (Apache-2.0 OR MIT).
// Encode: c3q4 path following google/wuffs lib/handsum (Apache-2.0 OR MIT).

#include "thumtoo/lqip.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

extern "C" {
#include "../external/basic_handsum_decode.c"
}

namespace thumtoo {
namespace {

constexpr int kHandsumC3Q4Size = 147;

uint8_t nibblify(int32_t v) {
  return static_cast<uint8_t>(std::max(0, std::min(15, v)));
}

void write_nibble(uint8_t* buf, int& bit_offset, uint8_t nibble) {
  buf[bit_offset >> 3] |= static_cast<uint8_t>((nibble & 15) << (bit_offset & 4));
  bit_offset += 4;
}

// Encode one 8×8 block as Handsum q=4 (16 2×2 tiles × 3 coeffs × 4 bits).
int encode_block_q4(uint8_t* buf, int bit_offset, const uint8_t src[64]) {
  for (int i = 0; i < 16; ++i) {
    int x = 2 * (i & 3);
    int y = 2 * (i >> 2);
    int j = (8 * y) + x;

    int32_t s0 = static_cast<int32_t>(src[j + 0]) - 0x80;
    int32_t s1 = static_cast<int32_t>(src[j + 1]) - 0x80;
    int32_t s2 = static_cast<int32_t>(src[j + 8]) - 0x80;
    int32_t s3 = static_cast<int32_t>(src[j + 9]) - 0x80;

    int32_t dct0 = (s0 + s1 + s2 + s3 + 1) >> 1;
    int32_t dct1 = (s0 - s1 + s2 - s3 + 1) >> 1;
    int32_t dct2 = (s0 + s1 - s2 - s3 + 1) >> 1;

    int32_t v0 = (dct0 + 256 + 0x11) / 0x22;
    write_nibble(buf, bit_offset, nibblify(v0));

    // Match Go: (dct + 8.5*8) / 8
    int32_t v1 = static_cast<int32_t>((dct1 + 68) / 8);  // 8.5*8 = 68
    write_nibble(buf, bit_offset, nibblify(v1));

    int32_t v2 = static_cast<int32_t>((dct2 + 68) / 8);
    write_nibble(buf, bit_offset, nibblify(v2));
  }
  return bit_offset;
}

void rgb_to_ycbcr(uint8_t r, uint8_t g, uint8_t b, uint8_t& y, uint8_t& cb,
                  uint8_t& cr) {
  // Same integer path as Go image/color RGBToYCbCr
  int yy = (19595 * r + 38470 * g + 7471 * b + 32768) >> 16;
  int u = ((-11056 * r - 21712 * g + 32768 * b + 32768) >> 16) + 128;
  int v = ((32768 * r - 27440 * g - 5328 * b + 32768) >> 16) + 128;
  y = static_cast<uint8_t>(std::max(0, std::min(255, yy)));
  cb = static_cast<uint8_t>(std::max(0, std::min(255, u)));
  cr = static_cast<uint8_t>(std::max(0, std::min(255, v)));
}

void scale_and_bias_chroma_up(uint8_t b[64]) {
  for (int i = 0; i < 64; ++i) {
    int v = (static_cast<int>(b[i]) * 2) - 0x78;
    b[i] = static_cast<uint8_t>(std::max(0, std::min(255, v)));
  }
}

// Box-filter scale RGB888 into a 16×16 RGB buffer (stretch, like Go BiLinear.Scale).
void scale_to_16x16(const uint8_t* rgb, int width, int height, uint8_t out[16 * 16 * 3]) {
  if (width <= 0 || height <= 0) {
    std::memset(out, 0, 16 * 16 * 3);
    return;
  }
  for (int dy = 0; dy < 16; ++dy) {
    for (int dx = 0; dx < 16; ++dx) {
      int sx = std::min(width - 1, (dx * width) / 16);
      int sy = std::min(height - 1, (dy * height) / 16);
      const uint8_t* p = rgb + (static_cast<size_t>(sy) * width + sx) * 3;
      uint8_t* d = out + (static_cast<size_t>(dy) * 16 + dx) * 3;
      d[0] = p[0];
      d[1] = p[1];
      d[2] = p[2];
    }
  }
}

void fill_block_from_rgb16(const uint8_t rgb16[16 * 16 * 3], int ox, int oy,
                           uint8_t yb[64], uint8_t cbb[64], uint8_t crb[64]) {
  for (int dy = 0; dy < 8; ++dy) {
    for (int dx = 0; dx < 8; ++dx) {
      int x = ox + dx;
      int y = oy + dy;
      const uint8_t* p = rgb16 + (static_cast<size_t>(y) * 16 + x) * 3;
      uint8_t Y, Cb, Cr;
      rgb_to_ycbcr(p[0], p[1], p[2], Y, Cb, Cr);
      int i = dy * 8 + dx;
      yb[i] = Y;
      cbb[i] = Cb;
      crb[i] = Cr;
    }
  }
}

void downsample_4x4_into(uint8_t* dst4, const uint8_t src8[64]) {
  // Average 2×2 → write into 4×4 region of an 8×8 chroma plane (caller places).
  for (int y = 0; y < 4; ++y) {
    for (int x = 0; x < 4; ++x) {
      int i00 = (y * 2) * 8 + (x * 2);
      int sum = src8[i00] + src8[i00 + 1] + src8[i00 + 8] + src8[i00 + 9];
      dst4[y * 4 + x] = static_cast<uint8_t>((sum + 2) / 4);
    }
  }
}

}  // namespace

std::vector<std::uint8_t> handsum_encode_rgb888(const std::uint8_t* rgb,
                                                  int width, int height) {
  if (!rgb || width <= 0 || height <= 0) {
    return {};
  }

  uint8_t rgb16[16 * 16 * 3];
  scale_to_16x16(rgb, width, height, rgb16);

  uint8_t y0[64], y1[64], y2[64], y3[64];
  uint8_t cb0[64], cr0[64], cb1[64], cr1[64], cb2[64], cr2[64], cb3[64], cr3[64];
  fill_block_from_rgb16(rgb16, 0, 0, y0, cb0, cr0);
  fill_block_from_rgb16(rgb16, 8, 0, y1, cb1, cr1);
  fill_block_from_rgb16(rgb16, 0, 8, y2, cb2, cr2);
  fill_block_from_rgb16(rgb16, 8, 8, y3, cb3, cr3);

  // 4:2:0 chroma plane 8×8 from four 4×4 downsamples
  uint8_t cb[64] = {};
  uint8_t cr[64] = {};
  uint8_t tmp4[16];
  auto place = [&](uint8_t* plane, int ox, int oy, const uint8_t src8[64]) {
    downsample_4x4_into(tmp4, src8);
    for (int y = 0; y < 4; ++y) {
      for (int x = 0; x < 4; ++x) {
        plane[(oy + y) * 8 + (ox + x)] = tmp4[y * 4 + x];
      }
    }
  };
  place(cb, 0, 0, cb0);
  place(cb, 4, 0, cb1);
  place(cb, 0, 4, cb2);
  place(cb, 4, 4, cb3);
  place(cr, 0, 0, cr0);
  place(cr, 4, 0, cr1);
  place(cr, 0, 4, cr2);
  place(cr, 4, 4, cr3);

  scale_and_bias_chroma_up(cb);
  scale_and_bias_chroma_up(cr);

  // Aspect ratio header (same formula as Go handsum.Encode)
  uint8_t aspect = 0;
  if (width >= height) {
    int64_t a = ((static_cast<int64_t>(height) * 32) + width) / (2 * static_cast<int64_t>(width));
    if (a <= 0) a = 1;
    aspect = static_cast<uint8_t>((a - 1) | 0x00);
  } else {
    int64_t a = ((static_cast<int64_t>(width) * 32) + height) / (2 * static_cast<int64_t>(height));
    if (a <= 0) a = 1;
    aspect = static_cast<uint8_t>((a - 1) | 0x10);
    if (aspect == 0x1F) aspect = 0x0F;
  }

  // Color=3 (RGB), Quality=4 → Magic1 path: buf[0]=0xFE, buf[1] has low bit
  // From Go: Magic0 = "\xFE\xD6"; for RGB: buf[0]=Magic0[0]; buf[1]=Magic0[1]|((c-1)>>1);
  // c=3 → (c-1)>>1 = 1; buf[2] = ((c-1)<<7)|((q-1)<<5)|aspect
  std::vector<std::uint8_t> buf(kHandsumC3Q4Size, 0);
  buf[0] = 0xFE;
  buf[1] = static_cast<uint8_t>(0xD6 | ((3 - 1) >> 1));
  buf[2] = static_cast<uint8_t>(((3 - 1) << 7) | ((4 - 1) << 5) | aspect);

  int bit = 24;
  bit = encode_block_q4(buf.data(), bit, y0);
  bit = encode_block_q4(buf.data(), bit, y1);
  bit = encode_block_q4(buf.data(), bit, y2);
  bit = encode_block_q4(buf.data(), bit, y3);
  bit = encode_block_q4(buf.data(), bit, cb);
  bit = encode_block_q4(buf.data(), bit, cr);
  (void)bit;

  return buf;
}

std::optional<LqipRgba> handsum_decode(std::span<const std::uint8_t> bytes) {
  if (bytes.size() < 3) {
    return std::nullopt;
  }
  basic_handsum_decode__pixel_buffer pb{};
  int rc = basic_handsum_decode__decode(&pb, bytes.data(), bytes.size());
  if (rc != BASIC_HANDSUM_DECODE__RESULT__OK || pb.width == 0 || pb.height == 0) {
    return std::nullopt;
  }
  LqipRgba out;
  out.width = static_cast<int>(pb.width);
  out.height = static_cast<int>(pb.height);
  out.rgba.resize(static_cast<size_t>(out.width) * out.height * 4);
  // Decoder produces BGRA; convert to RGBA
  for (int y = 0; y < out.height; ++y) {
    for (int x = 0; x < out.width; ++x) {
      size_t si = (static_cast<size_t>(y) * pb.width + x) * 4;
      size_t di = (static_cast<size_t>(y) * out.width + x) * 4;
      out.rgba[di + 0] = pb.bgra_pixels[si + 2];
      out.rgba[di + 1] = pb.bgra_pixels[si + 1];
      out.rgba[di + 2] = pb.bgra_pixels[si + 0];
      out.rgba[di + 3] = pb.bgra_pixels[si + 3];
    }
  }
  return out;
}

std::optional<LqipRgba> lqip_decode_rgba(std::span<const std::uint8_t> bytes) {
  if (bytes.size() >= 2 && bytes[0] == 0xFE && (bytes[1] & 0xFE) == 0xD6) {
    return handsum_decode(bytes);
  }
  return thumbhash_decode_rgba(bytes);
}

}  // namespace thumtoo
