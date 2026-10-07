#include <gtest/gtest.h>

#include <surf/convert.hpp>
#include <surf/pixel.hpp>

using namespace surf;

TEST(ConvertTest, convert_value)
{
  EXPECT_EQ(RGB32Pixel::max(), 4294967295);
  EXPECT_EQ((convert_value<Color, RGB32Pixel>(1.0f)), 4294967295);
}

TEST(ConvertTest, convert_value__rounding)
{
  EXPECT_EQ((convert_value<Color, RGB8Pixel>(0.5f)), 128);
  EXPECT_EQ((convert_value<Color, RGB8Pixel>(0.999f)), 255);
  EXPECT_EQ((convert_value<Color, RGB8Pixel>(-1.0f)), 0);
  EXPECT_EQ((convert_value<Color, RGB8Pixel>(2.0f)), 255);
  EXPECT_EQ((convert_value<Color, RGB32Pixel>(-1.0f)), 0u);
  EXPECT_EQ((convert_value<Color, RGB32Pixel>(2.0f)), 4294967295u);
  EXPECT_EQ(f2value<RGB8Pixel>(0.5f), 128);
  EXPECT_EQ(Color(0.5f, 0.5f, 0.5f).r8(), 128);
}

TEST(ConvertTest, roundtrip)
{
  for (int i = 0; i < 256; ++i) {
    RGB8Pixel const pixel{static_cast<uint8_t>(i), static_cast<uint8_t>(255 - i), 0};
    EXPECT_EQ((convert<Color, RGB8Pixel>(convert<RGB8Pixel, Color>(pixel))), pixel);
  }

  for (int i = 0; i < 65536; i += 7) {
    RGB16Pixel const pixel{static_cast<uint16_t>(i), static_cast<uint16_t>(65535 - i), 0};
    EXPECT_EQ((convert<RGB32fPixel, RGB16Pixel>(convert<RGB16Pixel, RGB32fPixel>(pixel))), pixel);
  }
}

TEST(ConvertTest, convert_rgb)
{
  RGB32Pixel rgb32 = convert<Color, RGB32Pixel>(Color(1.0f, 1.0f, 1.0f));
  EXPECT_EQ(rgb32.r, 4294967295);
  EXPECT_EQ(rgb32.g, 4294967295);
  EXPECT_EQ(rgb32.b, 4294967295);

  RGB8Pixel rgb8 = convert<RGB32Pixel, RGB8Pixel>(rgb32);
  EXPECT_EQ(rgb8.r, 255);
  EXPECT_EQ(rgb8.g, 255);
  EXPECT_EQ(rgb8.b, 255);
}

TEST(ConvertTest, convert_rgba)
{
  RGBA32Pixel rgb32 = convert<Color, RGBA32Pixel>(Color(1.0f, 1.0f, 1.0f));
  EXPECT_EQ(rgb32.r, 4294967295);
  EXPECT_EQ(rgb32.g, 4294967295);
  EXPECT_EQ(rgb32.b, 4294967295);
  EXPECT_EQ(rgb32.a, 4294967295);

  RGBA8Pixel rgb8 = convert<RGBA32Pixel, RGBA8Pixel>(rgb32);
  EXPECT_EQ(rgb8.r, 255);
  EXPECT_EQ(rgb8.g, 255);
  EXPECT_EQ(rgb8.b, 255);
  EXPECT_EQ(rgb8.a, 255);
}

/* EOF */

