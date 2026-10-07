#include <gtest/gtest.h>

#include <array>

#include <surf/filter.hpp>
#include <surf/pixel_data.hpp>

using namespace surf;

TEST(FilterTest, apply_add)
{
  PixelData<RGBA8Pixel> img({1, 1}, RGBA8Pixel{100, 200, 0, 50});

  apply_add(img, 0.5f);
  EXPECT_EQ(img.get_pixel({0, 0}), (RGBA8Pixel{228, 255, 128, 50}));

  apply_add(img, -1.0f);
  EXPECT_EQ(img.get_pixel({0, 0}), (RGBA8Pixel{0, 0, 0, 50}));
}

TEST(FilterTest, apply_threshold)
{
  PixelData<RGBA8Pixel> img({1, 1}, RGBA8Pixel{100, 200, 0, 50});
  apply_threshold(img, Color(0.5f, 0.5f, 0.5f));
  EXPECT_EQ(img.get_pixel({0, 0}), (RGBA8Pixel{0, 255, 0, 50}));

  PixelData<RGBA32fPixel> imgf({1, 1}, RGBA32fPixel{0.25f, 0.75f, 0.0f, 0.5f});
  apply_threshold(imgf, Color(0.5f, 0.5f, 0.5f));
  EXPECT_EQ(imgf.get_pixel({0, 0}), (RGBA32fPixel{0.0f, 1.0f, 0.0f, 0.5f}));
}

TEST(FilterTest, apply_grayscale)
{
  PixelData<RGBA8Pixel> img({1, 1}, RGBA8Pixel{30, 60, 90, 50});
  apply_grayscale(img);
  EXPECT_EQ(img.get_pixel({0, 0}), (RGBA8Pixel{60, 60, 60, 50}));

  uint32_t const v = 0xf0000000u;
  PixelData<RGB32Pixel> img32({1, 1}, RGB32Pixel{v, v, v});
  apply_grayscale(img32);
  EXPECT_EQ(img32.get_pixel({0, 0}), (RGB32Pixel{v, v, v}));
}

TEST(FilterTest, apply_lut)
{
  std::array<uint8_t, 256> lut;
  for (size_t i = 0; i < lut.size(); ++i) {
    lut[i] = static_cast<uint8_t>(255 - i);
  }

  PixelData<RGBA8Pixel> img({1, 1}, RGBA8Pixel{0, 100, 255, 50});
  apply_lut(img, lut.data());
  EXPECT_EQ(img.get_pixel({0, 0}), (RGBA8Pixel{255, 155, 0, 50}));

  PixelData<RGB16Pixel> img16({1, 1});
  EXPECT_THROW(apply_lut(img16, lut.data()), std::invalid_argument);
}

TEST(FilterTest, apply_hsv_keeps_alpha)
{
  PixelData<RGBA8Pixel> img({1, 1}, RGBA8Pixel{255, 0, 0, 50});
  apply_hsv(img, 0.0f, 0.0f, 0.0f);
  EXPECT_EQ(img.get_pixel({0, 0}).a, 50);
}

TEST(FilterTest, apply_offset_empty)
{
  PixelData<RGB8Pixel> img;
  apply_offset(img, geom::ioffset(1, 1));
  EXPECT_TRUE(img.get_size().is_null());
}

/* EOF */
