#include <iostream>
#include <gtest/gtest.h>

#include <geom/rect.hpp>
#include <geom/io.hpp>

#include <surf/palette.hpp>
#include <surf/software_surface.hpp>
#include <surf/transform.hpp>

using namespace surf;

TEST(SoftwareSurfaceTest, default_is_valid)
{
  SoftwareSurface const pixel_data;

  EXPECT_EQ(geom::isize(0, 0), pixel_data.get_size());

#if 0
  EXPECT_EQ(geom::isize(0, 0), halve(pixel_data).get_size());
  EXPECT_EQ(geom::isize(0, 0), scale(pixel_data, geom::isize(32, 16)).get_size());
  EXPECT_EQ(geom::isize(0, 0), crop(pixel_data, geom::irect(0, 0, 16, 16)).get_size());

  transform(pixel_data, Transform::ROTATE_0);
  rotate90(pixel_data);
  rotate180(pixel_data);
  rotate270(pixel_data);
  flip_horizontal(pixel_data);
  flip_vertical(pixel_data);
  SUCCEED();

  to_rgb(pixel_data);
  average_color(pixel_data);
#endif
}

TEST(SoftwareSurfaceTest, assignment)
{
  SoftwareSurface lhs(PixelData<RGBPixel>(geom::isize(32, 16)));
  SoftwareSurface rhs(PixelData<RGBPixel>(geom::isize(64, 32)));
  SoftwareSurface tmp;

  tmp = lhs;
  lhs = rhs;
  rhs = tmp;

  EXPECT_EQ(geom::isize(64, 32), lhs.get_size());
  EXPECT_EQ(geom::isize(32, 16), rhs.get_size());
}

TEST(SoftwareSurfaceTest, move)
{
  SoftwareSurface lhs(PixelData<RGBPixel>(geom::isize(32, 16)));
  SoftwareSurface rhs(PixelData<RGBPixel>(geom::isize(64, 32)));
  SoftwareSurface tmp;

  tmp = std::move(lhs);
  lhs = std::move(rhs);
  rhs = std::move(tmp);

  EXPECT_EQ(geom::isize(64, 32), lhs.get_size());
  EXPECT_EQ(geom::isize(32, 16), rhs.get_size());

  // this will crash as SoftwareSurface::m_pixel_data goes nullptr
  // EXPECT_EQ(geom::isize(32, 16), tmp.get_size());
}

TEST(SoftwareSurfaceTest, convert)
{
  SoftwareSurface const lhs(PixelData<RGBPixel>(geom::isize(32, 16)));
  SoftwareSurface rhs = convert(lhs, PixelFormat::RGBA8);

  EXPECT_EQ(lhs.get_size(), rhs.get_size());
}

TEST(SoftwareSurfaceTest, blit)
{
  SoftwareSurface const src(PixelData<RGB8Pixel>(geom::isize(4, 2), {255, 0, 0}));
  SoftwareSurface dst(PixelData<RGBA8Pixel>(geom::isize(8, 4), {0, 0, 0, 0}));

  blit(src, dst, geom::ipoint(1, 2));
}

TEST(SoftwareSurfaceTest, fill)
{
  SoftwareSurface const src(PixelData<RGB8Pixel>(geom::isize(4, 2), {255, 0, 0}));
  SoftwareSurface dst(PixelData<RGBA8Pixel>(geom::isize(8, 4), {0, 0, 0, 0}));

  fill(dst, palette::white);
}

TEST(SoftwareSurfaceTest, fill_rect)
{
  SoftwareSurface const src(PixelData<RGB8Pixel>(geom::isize(4, 2), {255, 0, 0}));
  SoftwareSurface dst(PixelData<RGBA8Pixel>(geom::isize(8, 4), {0, 0, 0, 0}));

  fill_rect(dst, geom::irect(1, 2, 4, 4), palette::white);
}

TEST(SoftwareSurfaceTest, get_view)
{
  SoftwareSurface const src(PixelData<RGB8Pixel>(geom::isize(8, 6), {255, 0, 0}));
  SoftwareSurface view(src.get_view(geom::irect(2, 2, 5, 5)));
  fill(view, Color(0, 0, 0, 0));
}

TEST(SoftwareSurfaceTest, create_view)
{
  PixelData<RGB8Pixel> pixeldata(geom::isize(8, 4), {255, 0, 0});
  SoftwareSurface view = SoftwareSurface::create_view(pixeldata);
  fill(view, Color(0, 0, 0, 0));
}

TEST(SoftwareSurfaceTest, create_view__const)
{
  PixelData<RGB8Pixel> const pixeldata(geom::isize(8, 4), {255, 0, 0});
  SoftwareSurface view = SoftwareSurface::create_view(pixeldata);
  fill(view, Color(0, 0, 0, 0));
}

TEST(SoftwareSurfaceTest, empty_copy)
{
  SoftwareSurface const empty;
  SoftwareSurface copy(empty);
  EXPECT_EQ(copy.get_format(), PixelFormat::NONE);
  EXPECT_EQ(copy, empty);

  SoftwareSurface surface = SoftwareSurface::create(PixelFormat::RGB8, {4, 4});
  EXPECT_NE(surface, empty);
  surface = empty;
  EXPECT_EQ(surface.get_format(), PixelFormat::NONE);

  EXPECT_THROW(empty.get_pixel({0, 0}), std::runtime_error);
  EXPECT_THROW(empty.get_view(geom::irect(0, 0, 1, 1)), std::runtime_error);
  EXPECT_THROW(empty.get_pixel_data(), std::runtime_error);
}

TEST(SoftwareSurfaceTest, create_view__padded_pitch)
{
  // 3x2 RGB8 with rows padded to 12 bytes
  std::vector<uint8_t> data(24, 0);
  data[12 + 6 + 0] = 255;

  SoftwareSurface const surface = SoftwareSurface::create_view(PixelFormat::RGB8, {3, 2}, data.data(), 12);
  EXPECT_EQ(surface.get_pitch(), 12);
  EXPECT_EQ(surface.get_pixel({2, 1}), Color(1.0f, 0.0f, 0.0f));
  EXPECT_EQ(surface.get_view(geom::irect(2, 1, 3, 2)).get_pixel({0, 0}), Color(1.0f, 0.0f, 0.0f));
}

TEST(SoftwareSurfaceTest, create_64f_unsupported)
{
  EXPECT_THROW(SoftwareSurface::create(PixelFormat::RGB64f, {1, 1}), std::invalid_argument);
  EXPECT_THROW(SoftwareSurface::create(PixelFormat::NONE, {1, 1}), std::invalid_argument);
}

/* EOF */
