#include <iostream>
#include <gtest/gtest.h>

#include "surface/software_surface.hpp"
#include "surface/rgb.hpp"
#include "math/rect.hpp"

TEST(SoftwareSurfaceTest, default_is_valid)
{
  SoftwareSurface surface;

  EXPECT_EQ(Size(0, 0), surface.get_size());

  EXPECT_EQ(Size(0, 0), surface.halve().get_size());
  EXPECT_EQ(Size(32, 16), surface.scale(Size(32, 16)).get_size());
  EXPECT_EQ(Size(0, 0), surface.crop(Rect(0, 0, 16, 16)).get_size());

  surface.transform(SoftwareSurface::kRot0);
  surface.rotate90();
  surface.rotate180();
  surface.rotate270();
  surface.vflip();
  surface.hflip();
  surface.to_rgb();
  surface.get_average_color();

  surface.get_pixel_data();
}

TEST(SoftwareSurfaceTest, assignment)
{
  SoftwareSurface lhs(PixelData(PixelData::RGB_FORMAT, Size(32, 16)));
  SoftwareSurface rhs(PixelData(PixelData::RGB_FORMAT, Size(64, 32)));
  SoftwareSurface tmp;

  tmp = lhs;
  lhs = rhs;
  rhs = tmp;

  EXPECT_EQ(Size(64, 32), lhs.get_size());
  EXPECT_EQ(Size(32, 16), rhs.get_size());
  EXPECT_EQ(Size(32, 16), tmp.get_size());
}

TEST(SoftwareSurfaceTest, move)
{
  SoftwareSurface lhs(PixelData(PixelData::RGB_FORMAT, Size(32, 16)));
  SoftwareSurface rhs(PixelData(PixelData::RGB_FORMAT, Size(64, 32)));
  SoftwareSurface tmp;

  tmp = std::move(lhs);
  lhs = std::move(rhs);
  rhs = std::move(tmp);

  EXPECT_EQ(Size(64, 32), lhs.get_size());
  EXPECT_EQ(Size(32, 16), rhs.get_size());

  // this will crash as SoftwareSurface::m_pixel_data goes nullptr
  // EXPECT_EQ(Size(32, 16), tmp.get_size());
}

/* EOF */
