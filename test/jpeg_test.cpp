#include <gtest/gtest.h>

#include <surf/software_surface.hpp>

#include "math/size.hpp"
#include <surf/plugins/jpeg.hpp>

using namespace surf;

TEST(JPEGTest, get_size)
{
  EXPECT_EQ(Size(64, 128), jpeg::get_size("test/data/images/rgb.jpg"));
  EXPECT_EQ(Size(64, 128), jpeg::get_size("test/data/images/grayscale.jpg"));
  EXPECT_EQ(Size(64, 128), jpeg::get_size("test/data/images/cmyk.jpg"));
}

TEST(JPEGTest, load_from_file)
{
  EXPECT_NO_THROW({ jpeg::load_from_file("test/data/images/rgb.jpg"); });
  EXPECT_NO_THROW({ jpeg::load_from_file("test/data/images/grayscale.jpg"); });
  EXPECT_NO_THROW({ jpeg::load_from_file("test/data/images/cmyk.jpg"); });
}

/* EOF */
