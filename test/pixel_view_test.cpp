#include <iostream>
#include <gtest/gtest.h>

#include <geom/rect.hpp>

#include <surf/blend.hpp>
#include <surf/blit.hpp>
#include <surf/fill.hpp>
#include <surf/color.hpp>
#include <surf/pixel_data.hpp>
#include <surf/sdl.hpp>
#include <surf/transform.hpp>
#include <surf/io.hpp>

using namespace surf;

TEST(PixelViewTest, get_view)
{
  PixelData<RGBPixel> const black(geom::isize(16, 8), RGBPixel{0, 0, 0});

  PixelData<RGBPixel> expected(geom::isize(16, 8), RGBPixel{0xde, 0xad, 0xff});
  fill_rect(expected, geom::irect(1, 1, 15, 7), RGBPixel{0, 0, 0});

  PixelData<RGBPixel> pixeldata(geom::isize(16, 8), RGBPixel{0xde, 0xad, 0xff});
  PixelView<RGBPixel> pixelview = pixeldata.get_view(geom::irect(1, 1, 15, 7));
  blit(black, pixelview, geom::ipoint(0, 0));

  EXPECT_EQ(pixeldata, expected);
}

TEST(PixelViewTest, from_pitch)
{
  // 3x2 RGB8 with rows padded to 12 bytes
  std::vector<uint8_t> data(24, 0);
  data[12 + 6 + 0] = 1;
  data[12 + 6 + 1] = 2;
  data[12 + 6 + 2] = 3;

  PixelView<RGB8Pixel> view = PixelView<RGB8Pixel>::from_pitch(
    {3, 2}, reinterpret_cast<RGB8Pixel*>(data.data()), 12);
  EXPECT_EQ(view.get_pitch(), 12);
  EXPECT_EQ(view.get_pixel({2, 1}), (RGB8Pixel{1, 2, 3}));

  // sub views keep the pitch
  PixelView<RGB8Pixel> sub = view.get_view(geom::irect(1, 0, 3, 2));
  EXPECT_EQ(sub.get_pitch(), 12);
  EXPECT_EQ(sub.get_pixel({1, 1}), (RGB8Pixel{1, 2, 3}));

  sub.put_pixel({0, 1}, RGB8Pixel{4, 5, 6});
  EXPECT_EQ(data[12 + 3], 4);
}

TEST(PixelViewTest, from_pitch__invalid)
{
  std::vector<uint16_t> data(64);
  EXPECT_THROW(PixelView<RGB8Pixel>::from_pitch({4, 2}, reinterpret_cast<RGB8Pixel*>(data.data()), 11),
               std::invalid_argument);
  // RGB16 needs a pitch that is a multiple of two
  EXPECT_THROW(PixelView<RGB16Pixel>::from_pitch({2, 2}, reinterpret_cast<RGB16Pixel*>(data.data()), 13),
               std::invalid_argument);
  EXPECT_NO_THROW(PixelView<RGB16Pixel>::from_pitch({2, 2}, reinterpret_cast<RGB16Pixel*>(data.data()), 14));
}

/* EOF */
