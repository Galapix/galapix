#include <cstring>

#include <gtest/gtest.h>

#include <surf/surf.hpp>

#include "plugins/stb.hpp"

using namespace surf;

TEST(StbTest, load_keeps_pixel_format)
{
  EXPECT_EQ(stb::load_from_file("test/data/rgb.png").get_format(), PixelFormat::RGB8);
  EXPECT_EQ(stb::load_from_file("test/data/rgba.png").get_format(), PixelFormat::RGBA8);
  EXPECT_EQ(stb::load_from_file("test/data/rgb.jpg").get_format(), PixelFormat::RGB8);
}

TEST(StbTest, load_error)
{
  EXPECT_THROW(stb::load_from_file("test/data/does-not-exist.png"), std::runtime_error);
  std::vector<uint8_t> const garbage{1, 2, 3, 4};
  EXPECT_THROW(stb::load_from_mem(garbage), std::runtime_error);
}

TEST(StbTest, png_roundtrip)
{
  SoftwareSurface const src = stb::load_from_file("test/data/rgba.png");
  SoftwareSurface const dst = stb::load_from_mem(stb::save_png(src));

  ASSERT_EQ(dst.get_size(), src.get_size());
  ASSERT_EQ(dst.get_format(), src.get_format());
  for (int y = 0; y < src.get_height(); ++y) {
    EXPECT_EQ(std::memcmp(dst.get_row_data(y), src.get_row_data(y), static_cast<size_t>(src.get_width()) * 4), 0);
  }
}

TEST(StbTest, png_converts_to_8_bit)
{
  SoftwareSurface const src = convert(stb::load_from_file("test/data/rgba.png"), PixelFormat::RGBA16);
  EXPECT_EQ(stb::load_from_mem(stb::save_png(src)).get_format(), PixelFormat::RGBA8);
}

TEST(StbTest, jpeg_roundtrip)
{
  SoftwareSurface const src = stb::load_from_file("test/data/rgb.png");
  SoftwareSurface const dst = stb::load_from_mem(stb::save_jpeg(src, 90));

  EXPECT_EQ(dst.get_size(), src.get_size());
  EXPECT_EQ(dst.get_format(), PixelFormat::RGB8);
}

TEST(StbTest, factory_uses_stb)
{
  SoftwareSurface const surface = SoftwareSurface::from_file("test/data/rgb.jpg");
  EXPECT_EQ(surface.get_format(), PixelFormat::RGB8);
}

/* EOF */
