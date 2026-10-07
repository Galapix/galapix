// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
//
// ThumbHash port of https://github.com/evanw/thumbhash (MIT).

#include "thumtoo/lqip.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace thumtoo {
namespace {

constexpr float kPi = 3.14159265358979323846f;

}  // namespace

std::vector<std::uint8_t> thumbhash_encode_rgba(int w, int h,
                                                std::span<const std::uint8_t> rgba) {
  if (w <= 0 || h <= 0 || w > 100 || h > 100) {
    throw std::invalid_argument("thumbhash_encode_rgba: size must be 1..100");
  }
  const std::size_t need = static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4u;
  if (rgba.size() < need) {
    throw std::invalid_argument("thumbhash_encode_rgba: rgba buffer too small");
  }

  float avg_r = 0, avg_g = 0, avg_b = 0, avg_a = 0;
  for (int i = 0, j = 0; i < w * h; ++i, j += 4) {
    const float alpha = rgba[static_cast<std::size_t>(j + 3)] / 255.0f;
    avg_r += alpha / 255.0f * rgba[static_cast<std::size_t>(j + 0)];
    avg_g += alpha / 255.0f * rgba[static_cast<std::size_t>(j + 1)];
    avg_b += alpha / 255.0f * rgba[static_cast<std::size_t>(j + 2)];
    avg_a += alpha;
  }
  if (avg_a > 0) {
    avg_r /= avg_a;
    avg_g /= avg_a;
    avg_b /= avg_a;
  }

  const bool has_alpha = avg_a < static_cast<float>(w * h);
  const int l_limit = has_alpha ? 5 : 7;
  const int lx = std::max(1, static_cast<int>(std::lround(
                                 static_cast<float>(l_limit) * w / std::max(w, h))));
  const int ly = std::max(1, static_cast<int>(std::lround(
                                 static_cast<float>(l_limit) * h / std::max(w, h))));

  std::vector<float> l(static_cast<std::size_t>(w * h));
  std::vector<float> p(static_cast<std::size_t>(w * h));
  std::vector<float> q(static_cast<std::size_t>(w * h));
  std::vector<float> a(static_cast<std::size_t>(w * h));

  for (int i = 0, j = 0; i < w * h; ++i, j += 4) {
    const float alpha = rgba[static_cast<std::size_t>(j + 3)] / 255.0f;
    const float r =
        avg_r * (1.0f - alpha) + alpha / 255.0f * rgba[static_cast<std::size_t>(j + 0)];
    const float g =
        avg_g * (1.0f - alpha) + alpha / 255.0f * rgba[static_cast<std::size_t>(j + 1)];
    const float b =
        avg_b * (1.0f - alpha) + alpha / 255.0f * rgba[static_cast<std::size_t>(j + 2)];
    l[static_cast<std::size_t>(i)] = (r + g + b) / 3.0f;
    p[static_cast<std::size_t>(i)] = (r + g) / 2.0f - b;
    q[static_cast<std::size_t>(i)] = r - g;
    a[static_cast<std::size_t>(i)] = alpha;
  }

  auto encode_channel = [&](const std::vector<float>& channel, int nx, int ny) {
    float dc = 0;
    std::vector<float> ac;
    float scale = 0;
    std::vector<float> fx(static_cast<std::size_t>(w));
    for (int cy = 0; cy < ny; ++cy) {
      for (int cx = 0; cx * ny < nx * (ny - cy); ++cx) {
        float f = 0;
        for (int x = 0; x < w; ++x) {
          fx[static_cast<std::size_t>(x)] =
              std::cos(kPi / static_cast<float>(w) * static_cast<float>(cx) *
                       (static_cast<float>(x) + 0.5f));
        }
        for (int y = 0; y < h; ++y) {
          const float fy =
              std::cos(kPi / static_cast<float>(h) * static_cast<float>(cy) *
                       (static_cast<float>(y) + 0.5f));
          for (int x = 0; x < w; ++x) {
            f += channel[static_cast<std::size_t>(x + y * w)] *
                 fx[static_cast<std::size_t>(x)] * fy;
          }
        }
        f /= static_cast<float>(w * h);
        if (cx || cy) {
          ac.push_back(f);
          scale = std::max(scale, std::fabs(f));
        } else {
          dc = f;
        }
      }
    }
    if (scale > 0) {
      for (float& v : ac) {
        v = 0.5f + 0.5f / scale * v;
      }
    }
    return std::tuple<float, std::vector<float>, float>{dc, std::move(ac), scale};
  };

  auto [l_dc, l_ac, l_scale] =
      encode_channel(l, std::max(3, lx), std::max(3, ly));
  auto [p_dc, p_ac, p_scale] = encode_channel(p, 3, 3);
  auto [q_dc, q_ac, q_scale] = encode_channel(q, 3, 3);
  float a_dc = 1;
  float a_scale = 0;
  std::vector<float> a_ac;
  if (has_alpha) {
    auto enc_a = encode_channel(a, 5, 5);
    a_dc = std::get<0>(enc_a);
    a_ac = std::move(std::get<1>(enc_a));
    a_scale = std::get<2>(enc_a);
  }

  const bool is_landscape = w > h;
  const int header24 =
      static_cast<int>(std::lround(63.0f * l_dc)) |
      (static_cast<int>(std::lround(31.5f + 31.5f * p_dc)) << 6) |
      (static_cast<int>(std::lround(31.5f + 31.5f * q_dc)) << 12) |
      (static_cast<int>(std::lround(31.0f * l_scale)) << 18) |
      ((has_alpha ? 1 : 0) << 23);
  const int header16 =
      (is_landscape ? ly : lx) |
      (static_cast<int>(std::lround(63.0f * p_scale)) << 3) |
      (static_cast<int>(std::lround(63.0f * q_scale)) << 9) |
      ((is_landscape ? 1 : 0) << 15);

  std::vector<std::uint8_t> hash;
  hash.push_back(static_cast<std::uint8_t>(header24 & 255));
  hash.push_back(static_cast<std::uint8_t>((header24 >> 8) & 255));
  hash.push_back(static_cast<std::uint8_t>(header24 >> 16));
  hash.push_back(static_cast<std::uint8_t>(header16 & 255));
  hash.push_back(static_cast<std::uint8_t>(header16 >> 8));
  const int ac_start = has_alpha ? 6 : 5;
  if (has_alpha) {
    hash.push_back(static_cast<std::uint8_t>(
        static_cast<int>(std::lround(15.0f * a_dc)) |
        (static_cast<int>(std::lround(15.0f * a_scale)) << 4)));
  }

  // Pre-size AC region
  int ac_index = 0;
  auto append_ac = [&](const std::vector<float>& ac) {
    for (float f : ac) {
      const std::size_t idx =
          static_cast<std::size_t>(ac_start + (ac_index >> 1));
      if (hash.size() <= idx) {
        hash.resize(idx + 1, 0);
      }
      hash[idx] = static_cast<std::uint8_t>(
          hash[idx] |
          (static_cast<int>(std::lround(15.0f * f)) << ((ac_index & 1) << 2)));
      ++ac_index;
    }
  };
  append_ac(l_ac);
  append_ac(p_ac);
  append_ac(q_ac);
  if (has_alpha) {
    append_ac(a_ac);
  }
  return hash;
}

std::optional<LqipRgba> thumbhash_decode_rgba(std::span<const std::uint8_t> hash) {
  if (hash.size() < 5) {
    return std::nullopt;
  }

  const int header24 =
      hash[0] | (hash[1] << 8) | (hash[2] << 16);
  const int header16 = hash[3] | (hash[4] << 8);
  const float l_dc = static_cast<float>(header24 & 63) / 63.0f;
  const float p_dc = static_cast<float>((header24 >> 6) & 63) / 31.5f - 1.0f;
  const float q_dc = static_cast<float>((header24 >> 12) & 63) / 31.5f - 1.0f;
  const float l_scale = static_cast<float>((header24 >> 18) & 31) / 31.0f;
  const bool has_alpha = (header24 >> 23) != 0;
  const float p_scale = static_cast<float>((header16 >> 3) & 63) / 63.0f;
  const float q_scale = static_cast<float>((header16 >> 9) & 63) / 63.0f;
  const bool is_landscape = (header16 >> 15) != 0;
  const int lx = std::max(3, is_landscape ? (has_alpha ? 5 : 7) : (header16 & 7));
  const int ly = std::max(3, is_landscape ? (header16 & 7) : (has_alpha ? 5 : 7));
  const float a_dc = has_alpha ? static_cast<float>(hash[5] & 15) / 15.0f : 1.0f;
  const float a_scale = has_alpha ? static_cast<float>(hash[5] >> 4) / 15.0f : 0.0f;

  const int ac_start = has_alpha ? 6 : 5;
  int ac_index = 0;
  auto decode_channel = [&](int nx, int ny, float scale) {
    std::vector<float> ac;
    for (int cy = 0; cy < ny; ++cy) {
      for (int cx = cy ? 0 : 1; cx * ny < nx * (ny - cy); ++cx) {
        if (ac_start + (ac_index >> 1) >= static_cast<int>(hash.size())) {
          ac.push_back(0);
          ++ac_index;
          continue;
        }
        const float n =
            static_cast<float>(
                (hash[static_cast<std::size_t>(ac_start + (ac_index >> 1))] >>
                 ((ac_index & 1) << 2)) &
                15) /
                15.0f -
            0.5f;
        ac.push_back(n * scale);
        ++ac_index;
      }
    }
    return ac;
  };

  auto l_ac = decode_channel(lx, ly, l_scale);
  auto p_ac = decode_channel(3, 3, p_scale * 1.25f);
  auto q_ac = decode_channel(3, 3, q_scale * 1.25f);
  std::vector<float> a_ac;
  if (has_alpha) {
    a_ac = decode_channel(5, 5, a_scale);
  }

  // Approximate aspect ratio → output size (same as JS reference).
  const float ratio = static_cast<float>(lx) / static_cast<float>(ly);
  int w = 32;
  int h = 32;
  if (ratio > 1.0f) {
    h = std::max(1, static_cast<int>(std::lround(32.0f / ratio)));
  } else {
    w = std::max(1, static_cast<int>(std::lround(32.0f * ratio)));
  }

  LqipRgba out;
  out.width = w;
  out.height = h;
  out.rgba.resize(static_cast<std::size_t>(w * h * 4));

  auto fx = std::vector<std::vector<float>>(static_cast<std::size_t>(lx),
                                            std::vector<float>(static_cast<std::size_t>(w)));
  auto fy = std::vector<std::vector<float>>(static_cast<std::size_t>(ly),
                                            std::vector<float>(static_cast<std::size_t>(h)));
  for (int cx = 0; cx < lx; ++cx) {
    for (int x = 0; x < w; ++x) {
      fx[static_cast<std::size_t>(cx)][static_cast<std::size_t>(x)] =
          std::cos(kPi / static_cast<float>(w) * static_cast<float>(cx) *
                   (static_cast<float>(x) + 0.5f));
    }
  }
  for (int cy = 0; cy < ly; ++cy) {
    for (int y = 0; y < h; ++y) {
      fy[static_cast<std::size_t>(cy)][static_cast<std::size_t>(y)] =
          std::cos(kPi / static_cast<float>(h) * static_cast<float>(cy) *
                   (static_cast<float>(y) + 0.5f));
    }
  }

  // Reconstruct via DCT (ThumbHash reference algorithm).
  std::vector<float> l_ac2 = l_ac;
  std::vector<float> p_ac2 = p_ac;
  std::vector<float> q_ac2 = q_ac;
  std::vector<float> a_ac2 = a_ac;

  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      float lf = l_dc;
      float pf = p_dc;
      float qf = q_dc;
      float af = a_dc;

      int j = 0;
      for (int cy = 0; cy < ly; ++cy) {
        for (int cx = cy ? 0 : 1;
             cx * ly < lx * (ly - cy); ++cx, ++j) {
          const float basis =
              fx[static_cast<std::size_t>(cx)][static_cast<std::size_t>(x)] *
              fy[static_cast<std::size_t>(cy)][static_cast<std::size_t>(y)];
          lf += (j < static_cast<int>(l_ac2.size()) ? l_ac2[static_cast<std::size_t>(j)] : 0) *
                basis;
        }
      }
      j = 0;
      for (int cy = 0; cy < 3; ++cy) {
        for (int cx = cy ? 0 : 1; cx * 3 < 3 * (3 - cy); ++cx, ++j) {
          const float basis =
              std::cos(kPi / static_cast<float>(w) * static_cast<float>(cx) *
                       (static_cast<float>(x) + 0.5f)) *
              std::cos(kPi / static_cast<float>(h) * static_cast<float>(cy) *
                       (static_cast<float>(y) + 0.5f));
          pf += (j < static_cast<int>(p_ac2.size()) ? p_ac2[static_cast<std::size_t>(j)] : 0) *
                basis;
          qf += (j < static_cast<int>(q_ac2.size()) ? q_ac2[static_cast<std::size_t>(j)] : 0) *
                basis;
        }
      }
      if (has_alpha) {
        j = 0;
        for (int cy = 0; cy < 5; ++cy) {
          for (int cx = cy ? 0 : 1; cx * 5 < 5 * (5 - cy); ++cx, ++j) {
            const float basis =
                std::cos(kPi / static_cast<float>(w) * static_cast<float>(cx) *
                         (static_cast<float>(x) + 0.5f)) *
                std::cos(kPi / static_cast<float>(h) * static_cast<float>(cy) *
                         (static_cast<float>(y) + 0.5f));
            af += (j < static_cast<int>(a_ac2.size()) ? a_ac2[static_cast<std::size_t>(j)]
                                                      : 0) *
                  basis;
          }
        }
      }

      // LPQ → RGB (same as ThumbHash docs)
      float b = lf - 2.0f / 3.0f * pf;
      float r = (3.0f * lf - b + qf) / 2.0f;
      float g = r - qf;
      r = std::clamp(r, 0.0f, 1.0f);
      g = std::clamp(g, 0.0f, 1.0f);
      b = std::clamp(b, 0.0f, 1.0f);
      af = std::clamp(af, 0.0f, 1.0f);

      const std::size_t o = static_cast<std::size_t>((x + y * w) * 4);
      out.rgba[o + 0] = static_cast<std::uint8_t>(std::lround(255.0f * r));
      out.rgba[o + 1] = static_cast<std::uint8_t>(std::lround(255.0f * g));
      out.rgba[o + 2] = static_cast<std::uint8_t>(std::lround(255.0f * b));
      out.rgba[o + 3] = static_cast<std::uint8_t>(std::lround(255.0f * af));
    }
  }
  return out;
}

std::vector<std::uint8_t> thumbhash_encode_rgb888(const std::uint8_t* rgb,
                                                  int width, int height,
                                                  int max_edge) {
  if (!rgb || width <= 0 || height <= 0) {
    return {};
  }
  max_edge = std::clamp(max_edge, 8, 100);
  int tw = width;
  int th = height;
  if (std::max(tw, th) > max_edge) {
    if (tw >= th) {
      th = std::max(1, static_cast<int>(std::lround(
                           static_cast<double>(th) * max_edge / tw)));
      tw = max_edge;
    } else {
      tw = std::max(1, static_cast<int>(std::lround(
                           static_cast<double>(tw) * max_edge / th)));
      th = max_edge;
    }
  }

  std::vector<std::uint8_t> rgba(static_cast<std::size_t>(tw * th * 4));
  for (int y = 0; y < th; ++y) {
    const int sy = y * height / th;
    for (int x = 0; x < tw; ++x) {
      const int sx = x * width / tw;
      const std::size_t si =
          (static_cast<std::size_t>(sy) * static_cast<std::size_t>(width) +
           static_cast<std::size_t>(sx)) *
          3u;
      const std::size_t di =
          (static_cast<std::size_t>(y) * static_cast<std::size_t>(tw) +
           static_cast<std::size_t>(x)) *
          4u;
      rgba[di + 0] = rgb[si + 0];
      rgba[di + 1] = rgb[si + 1];
      rgba[di + 2] = rgb[si + 2];
      rgba[di + 3] = 255;
    }
  }
  try {
    return thumbhash_encode_rgba(tw, th, rgba);
  } catch (...) {
    return {};
  }
}

}  // namespace thumtoo
