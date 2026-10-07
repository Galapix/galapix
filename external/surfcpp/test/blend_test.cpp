#include <gtest/gtest.h>

#include <surf/blend.hpp>
#include <surf/blit.hpp>
#include <surf/convert.hpp>
#include <surf/pixel.hpp>
#include <surf/pixel_data.hpp>

using namespace surf;

namespace {

template<typename Pixel>
int count_pixels(PixelView<Pixel> const& view, Pixel const& pixel)
{
  int count = 0;
  for (int y = 0; y < view.get_height(); ++y) {
    for (int x = 0; x < view.get_width(); ++x) {
      if (view.get_pixel({x, y}) == pixel) {
        count += 1;
      }
    }
  }
  return count;
}

} // namespace

TEST(BlendTest, blend_rgba8)
{
  pixel_blend<RGBA8Pixel, RGB8Pixel> blend;
  EXPECT_EQ(blend(RGBA8Pixel{255, 0, 0, 255}, RGB8Pixel{0, 0, 255}), (RGB8Pixel{255, 0, 0}));
  EXPECT_EQ(blend(RGBA8Pixel{255, 0, 0, 0}, RGB8Pixel{0, 0, 255}), (RGB8Pixel{0, 0, 255}));

  pixel_blend<RGBA8Pixel, RGBA8Pixel> blend_rgba;
  EXPECT_EQ(blend_rgba(RGBA8Pixel{255, 0, 0, 0}, RGBA8Pixel{0, 0, 0, 0}), (RGBA8Pixel{0, 0, 0, 0}));
  EXPECT_EQ(blend_rgba(RGBA8Pixel{255, 0, 0, 255}, RGBA8Pixel{0, 0, 255, 255}), (RGBA8Pixel{255, 0, 0, 255}));
}

TEST(BlendTest, blend_rgba16_no_overflow)
{
  pixel_blend<RGBA16Pixel, RGBA16Pixel> blend;
  EXPECT_EQ(blend(RGBA16Pixel{65535, 0, 0, 65535}, RGBA16Pixel{0, 0, 65535, 65535}),
            (RGBA16Pixel{65535, 0, 0, 65535}));

  RGBA16Pixel const half = blend(RGBA16Pixel{65535, 0, 0, 32768}, RGBA16Pixel{0, 0, 65535, 65535});
  EXPECT_NEAR(half.r, 32768, 2);
  EXPECT_NEAR(half.b, 32767, 2);
  EXPECT_EQ(half.a, 65535);
}

TEST(BlendTest, blend_mixed_depth)
{
  // src is converted into the dst range before blending
  pixel_blend<RGBA8Pixel, RGB16Pixel> blend;
  EXPECT_EQ(blend(RGBA8Pixel{255, 0, 0, 255}, RGB16Pixel{0, 0, 65535}), (RGB16Pixel{65535, 0, 0}));

  pixel_blend<RGBA8Pixel, RGBA16Pixel> blend_rgba;
  EXPECT_EQ(blend_rgba(RGBA8Pixel{255, 0, 0, 255}, RGBA16Pixel{0, 0, 65535, 65535}), (RGBA16Pixel{65535, 0, 0, 65535}));
}

TEST(BlendTest, blend_float)
{
  pixel_blend<RGBA32fPixel, RGB32fPixel> blend;
  RGB32fPixel const result = blend(RGBA32fPixel{1.0f, 0.0f, 0.0f, 0.5f}, RGB32fPixel{0.0f, 0.0f, 1.0f});
  EXPECT_FLOAT_EQ(result.r, 0.5f);
  EXPECT_FLOAT_EQ(result.g, 0.0f);
  EXPECT_FLOAT_EQ(result.b, 0.5f);
}

TEST(BlendTest, add_rgba16_no_overflow)
{
  pixel_add<RGBA16Pixel, RGB16Pixel> add;
  EXPECT_EQ(add(RGBA16Pixel{65535, 65535, 0, 65535}, RGB16Pixel{0, 65535, 0}), (RGB16Pixel{65535, 65535, 0}));
}

TEST(BlendTest, blend_scaled__whole_src)
{
  PixelData<RGB8Pixel> src({2, 2}, RGB8Pixel{255, 0, 0});
  PixelData<RGB8Pixel> dst({8, 8}, RGB8Pixel{0, 0, 0});
  blend_scaled(pixel_copy<RGB8Pixel, RGB8Pixel>(), src, dst, geom::irect(0, 0, 4, 4));
  EXPECT_EQ(count_pixels(dst, RGB8Pixel{255, 0, 0}), 16);
}

TEST(BlendTest, blend_scaled__srcrect)
{
  PixelData<L8Pixel> src({4, 4});
  for (int y = 0; y < 4; ++y) {
    for (int x = 0; x < 4; ++x) {
      src.put_pixel({x, y}, L8Pixel{static_cast<uint8_t>(y * 4 + x)});
    }
  }

  // downscale the lower right 2x2 into a single pixel
  PixelData<L8Pixel> dst({1, 1});
  blend_scaled(pixel_copy<L8Pixel, L8Pixel>(), src, geom::irect(2, 2, 4, 4), dst, geom::irect(0, 0, 1, 1));
  EXPECT_EQ(dst.get_pixel({0, 0}).l, 10);

  // upscale the lower right 2x2 into 4x4
  PixelData<L8Pixel> dst2({4, 4});
  blend_scaled(pixel_copy<L8Pixel, L8Pixel>(), src, geom::irect(2, 2, 4, 4), dst2, geom::irect(0, 0, 4, 4));
  EXPECT_EQ(dst2.get_pixel({0, 0}).l, 10);
  EXPECT_EQ(dst2.get_pixel({3, 0}).l, 11);
  EXPECT_EQ(dst2.get_pixel({0, 3}).l, 14);
  EXPECT_EQ(dst2.get_pixel({3, 3}).l, 15);
}

TEST(BlendTest, blend_scaled__empty)
{
  PixelData<RGB8Pixel> src({2, 2}, RGB8Pixel{255, 0, 0});
  PixelData<RGB8Pixel> dst({4, 4}, RGB8Pixel{0, 0, 0});
  blend_scaled(pixel_copy<RGB8Pixel, RGB8Pixel>(), src, dst, geom::irect(0, 0, 0, 0));
  blend_scaled(pixel_copy<RGB8Pixel, RGB8Pixel>(), src, geom::irect(0, 0, 0, 0), dst, geom::irect(0, 0, 4, 4));
  blend_scaled(pixel_copy<RGB8Pixel, RGB8Pixel>(), src, dst, geom::irect(100, 100, 104, 104));
  EXPECT_EQ(count_pixels(dst, RGB8Pixel{0, 0, 0}), 16);
}

TEST(BlendTest, blit_outside)
{
  PixelData<RGB8Pixel> src({4, 4}, RGB8Pixel{255, 0, 0});
  PixelData<RGB8Pixel> dst({8, 8}, RGB8Pixel{0, 0, 0});
  PixelData<RGB16Pixel> dst16({8, 8});

  for (geom::ipoint const pos : {geom::ipoint(100, 0), geom::ipoint(0, 100),
                                 geom::ipoint(-100, 0), geom::ipoint(0, -100)}) {
    blit(src, dst, pos);
    blit(src, dst16, pos);
    blend(pixel_blend<RGB8Pixel, RGB8Pixel>(), src, dst, pos);
  }
  EXPECT_EQ(count_pixels(dst, RGB8Pixel{0, 0, 0}), 64);
}

TEST(BlendTest, blit_slow_negative_pos)
{
  PixelData<RGB8Pixel> src({4, 4}, RGB8Pixel{255, 0, 0});
  PixelData<RGB8Pixel> dst({8, 8}, RGB8Pixel{0, 0, 0});
  experimental::blit__slow(src, dst, geom::ipoint(-2, -2));
  EXPECT_EQ(count_pixels(dst, RGB8Pixel{255, 0, 0}), 4);
  EXPECT_EQ(dst.get_pixel({1, 1}), (RGB8Pixel{255, 0, 0}));
  EXPECT_EQ(dst.get_pixel({2, 2}), (RGB8Pixel{0, 0, 0}));
}

/* EOF */
