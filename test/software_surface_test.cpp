#include <iostream>
#include <gtest/gtest.h>

#include "util/software_surface.hpp"
#include "math/rgb.hpp"
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

/* EOF */
