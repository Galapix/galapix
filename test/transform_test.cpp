#include <gtest/gtest.h>

#include <surf/pixel_data.hpp>
#include <surf/transform.hpp>

using namespace surf;

namespace {

PixelData<L8Pixel> make_test_image()
{
  // 4x2
  // 0 1 2 3
  // 4 5 6 7
  PixelData<L8Pixel> img({4, 2});
  for (int y = 0; y < img.get_height(); ++y) {
    for (int x = 0; x < img.get_width(); ++x) {
      img.put_pixel({x, y}, L8Pixel{static_cast<uint8_t>(y * 4 + x)});
    }
  }
  return img;
}

} // namespace

TEST(TransformTest, rotate90)
{
  PixelData<L8Pixel> const img = rotate90(make_test_image());
  ASSERT_EQ(img.get_size(), geom::isize(2, 4));
  EXPECT_EQ(img.get_pixel({0, 0}).l, 4);
  EXPECT_EQ(img.get_pixel({1, 0}).l, 0);
  EXPECT_EQ(img.get_pixel({1, 3}).l, 3);
}

TEST(TransformTest, rotate270)
{
  PixelData<L8Pixel> const img = rotate270(make_test_image());
  ASSERT_EQ(img.get_size(), geom::isize(2, 4));
  EXPECT_EQ(img.get_pixel({0, 0}).l, 3);
  EXPECT_EQ(img.get_pixel({1, 0}).l, 7);
  EXPECT_EQ(img.get_pixel({0, 3}).l, 0);
  EXPECT_EQ(img, rotate90(rotate180(make_test_image())));
}

TEST(TransformTest, transform_flip)
{
  PixelData<L8Pixel> const src = make_test_image();
  EXPECT_EQ(transform(src, Transform::ROTATE_90_FLIP), flip_vertical(rotate90(src)));
  EXPECT_EQ(transform(src, Transform::ROTATE_270_FLIP), flip_vertical(rotate270(src)));
}

TEST(TransformTest, halve_no_overflow)
{
  uint32_t const v = 0xf0000000u;
  PixelData<RGBA32Pixel> src({2, 2}, RGBA32Pixel{v, v, v, v});
  PixelData<RGBA32Pixel> const dst = halve(src);
  ASSERT_EQ(dst.get_size(), geom::isize(1, 1));
  EXPECT_EQ(dst.get_pixel({0, 0}), (RGBA32Pixel{v, v, v, v}));
}

/* EOF */
