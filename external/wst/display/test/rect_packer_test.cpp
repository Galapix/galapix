// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <gtest/gtest.h>

#include <random>
#include <vector>

#include <wstdisplay/rect_packer.hpp>

using namespace wstdisplay;

namespace {

bool overlaps(geom::irect const& a, geom::irect const& b)
{
  return a.left() < b.right() && b.left() < a.right() &&
    a.top() < b.bottom() && b.top() < a.bottom();
}

} // namespace

TEST(RectPackerTest, exact_fill)
{
  RectPacker packer(geom::isize(64, 64));
  for (int i = 0; i < 16; ++i) {
    ASSERT_TRUE(packer.allocate(geom::isize(16, 16)).has_value()) << i;
  }
  EXPECT_FALSE(packer.allocate(geom::isize(1, 1)).has_value());
  EXPECT_FLOAT_EQ(packer.get_occupancy(), 1.0f);
}

TEST(RectPackerTest, rejects_oversized)
{
  RectPacker packer(geom::isize(64, 64));
  EXPECT_FALSE(packer.allocate(geom::isize(65, 1)).has_value());
  EXPECT_FALSE(packer.allocate(geom::isize(0, 10)).has_value());
}

TEST(RectPackerTest, random_rects_dont_overlap)
{
  RectPacker packer(geom::isize(512, 512));
  std::mt19937 rng(1234);
  std::uniform_int_distribution<int> dist(4, 64);

  std::vector<geom::irect> rects;
  for (int i = 0; i < 400; ++i) {
    if (auto rect = packer.allocate(geom::isize(dist(rng), dist(rng)))) {
      EXPECT_GE(rect->left(), 0);
      EXPECT_GE(rect->top(), 0);
      EXPECT_LE(rect->right(), 512);
      EXPECT_LE(rect->bottom(), 512);
      for (auto const& other : rects) {
        ASSERT_FALSE(overlaps(*rect, other));
      }
      rects.push_back(*rect);
    }
  }

  EXPECT_GT(rects.size(), 100u);
  EXPECT_GT(packer.get_occupancy(), 0.6f);
}

TEST(RectPackerTest, clear)
{
  RectPacker packer(geom::isize(16, 16));
  ASSERT_TRUE(packer.allocate(geom::isize(16, 16)));
  packer.clear();
  EXPECT_TRUE(packer.allocate(geom::isize(16, 16)));
}

/* EOF */
