#include <iostream>
#include <gtest/gtest.h>

#include <geom/rect.hpp>

#include <surf/blend.hpp>
#include <surf/blit.hpp>
#include <surf/color.hpp>
#include <surf/fill.hpp>
#include <surf/pixel_data.hpp>
#include <surf/sdl.hpp>
#include <surf/transform.hpp>
#include <surf/io.hpp>
#include <surf/save.hpp>


using namespace surf;

TEST(PixelDataTest, default_is_valid)
{
  PixelData<RGBPixel> const pixel_data;

  EXPECT_EQ(geom::isize(0, 0), pixel_data.get_size());
  EXPECT_EQ(geom::isize(0, 0), halve(pixel_data).get_size());
  EXPECT_EQ(geom::isize(32, 16), scale(pixel_data, geom::isize(32, 16)).get_size());
  EXPECT_EQ(geom::isize(0, 0), crop(pixel_data, geom::irect(0, 0, 16, 16)).get_size());

  transform(pixel_data, Transform::ROTATE_0);
  rotate90(pixel_data);
  rotate180(pixel_data);
  rotate270(pixel_data);
  flip_horizontal(pixel_data);
  flip_vertical(pixel_data);
#if 0
  to_rgb(pixel_data);
  average_color(pixel_data);
#endif
}

TEST(PixelDataTest, creation)
{
  PixelData<RGBPixel> pixeldata;
  EXPECT_TRUE(pixeldata.empty());

  pixeldata = PixelData<RGBPixel>(geom::isize(64, 32));
  EXPECT_FALSE(pixeldata.empty());
  EXPECT_EQ(pixeldata.get_size(), geom::isize(64, 32));
}

TEST(PixelDataTest, equality)
{
  PixelData<RGBPixel> const black(geom::isize(64, 32), RGBPixel{0, 0, 0});
  PixelData<RGBPixel> const white(geom::isize(64, 32), RGBPixel{255, 128, 64});

  PixelData<RGBPixel> const pixeldata(geom::isize(64, 32), RGBPixel{255, 128, 64});
  EXPECT_FALSE(pixeldata.empty());

  EXPECT_EQ(pixeldata.get_size(), geom::isize(64, 32));
  EXPECT_EQ(pixeldata, pixeldata);
  EXPECT_EQ(pixeldata, white);
  EXPECT_NE(pixeldata, black);

  PixelData<RGBAPixel> pixeldata_rgba(geom::isize(64, 32), RGBAPixel{255, 128, 64, 255});
  EXPECT_NE(pixeldata, pixeldata_rgba);
}

TEST(PixelDataTest, blit)
{
  PixelData<RGBPixel> const white(geom::isize(4, 3), RGBPixel{255, 255, 255});

  PixelData<RGBPixel> pixeldata(geom::isize(8, 6), RGBPixel{255, 0, 0});
  PixelData<RGBPixel> pixeldata_expected(geom::isize(8, 6), RGBPixel{255, 255, 255});

  blit(white, pixeldata, geom::ipoint(0, 0));
  blit(white, pixeldata, geom::ipoint(4, 0));
  blit(white, pixeldata, geom::ipoint(0, 3));
  blit(white, pixeldata, geom::ipoint(4, 3));

  EXPECT_EQ(pixeldata, pixeldata_expected);
}

TEST(PixelDataTest, blit_to__srcrect)
{
  PixelData<RGBPixel> const white(geom::isize(8, 6), RGBPixel{255, 255, 255});

  PixelData<RGBPixel> pixeldata(geom::isize(8, 6), RGBPixel{255, 0, 0});
  PixelData<RGBPixel> pixeldata_expected(geom::isize(8, 6), RGBPixel{255, 255, 255});

  blit(white, geom::irect(geom::ipoint(1, 1), geom::isize(4, 3)), pixeldata, geom::ipoint(0, 0));
  blit(white, geom::irect(geom::ipoint(2, 1), geom::isize(4, 3)), pixeldata, geom::ipoint(4, 0));
  blit(white, geom::irect(geom::ipoint(3, 3), geom::isize(4, 3)), pixeldata, geom::ipoint(0, 3));
  blit(white, geom::irect(geom::ipoint(4, 3), geom::isize(4, 3)), pixeldata, geom::ipoint(4, 3));

  EXPECT_EQ(pixeldata, pixeldata_expected);
}

TEST(PixelDataTest, blit_to__convert)
{
  PixelData<RGBAPixel> const white(geom::isize(4, 3), RGBAPixel{255, 255, 255, 255});

  PixelData<RGBPixel> pixeldata(geom::isize(8, 6), RGBPixel{255, 0, 0});
  PixelData<RGBPixel> const pixeldata_expected(geom::isize(8, 6), RGBPixel{255, 255, 255});

  blit(white, pixeldata, geom::ipoint(0, 0));
  blit(white, pixeldata, geom::ipoint(4, 0));
  blit(white, pixeldata, geom::ipoint(0, 3));
  blit(white, pixeldata, geom::ipoint(4, 3));

  EXPECT_EQ(pixeldata, pixeldata_expected);
}

TEST(PixelDataTest, blit_to__srcrect_convert)
{
  PixelData<RGBAPixel> const white(geom::isize(8, 6), RGBAPixel{255, 255, 255, 255});

  PixelData<RGBPixel> pixeldata(geom::isize(8, 6), RGBPixel{255, 0, 0});
  PixelData<RGBPixel> pixeldata_expected(geom::isize(8, 6), RGBPixel{255, 255, 255});

  blit(white, geom::irect(geom::ipoint(1, 1), geom::isize(4, 3)), pixeldata, geom::ipoint(0, 0));
  blit(white, geom::irect(geom::ipoint(2, 1), geom::isize(4, 3)), pixeldata, geom::ipoint(4, 0));
  blit(white, geom::irect(geom::ipoint(3, 3), geom::isize(4, 3)), pixeldata, geom::ipoint(0, 3));
  blit(white, geom::irect(geom::ipoint(4, 3), geom::isize(4, 3)), pixeldata, geom::ipoint(4, 3));

  EXPECT_EQ(pixeldata, pixeldata_expected);
}

TEST(PixelDataTest, convert)
{
  PixelData<RGBPixel> const pixeldata_rgb(geom::isize(64, 32));
  PixelData<RGBAPixel> const pixeldata_rgba = pixeldata_rgb.convert_to<RGBAPixel>();

  EXPECT_EQ(pixeldata_rgb.get_size(), pixeldata_rgba.get_size());
}

TEST(PixelDataTest, blend)
{
  PixelData<RGBAPixel> dst(geom::isize(512, 512), RGBAPixel{255, 255, 255, 255});
  //PixelData<RGBPixel> dst(geom::isize(512, 512), RGBPixel{255, 255, 255});

  SoftwareSurface surface = SoftwareSurface::from_file("test/data/rgba.png");
  PixelView<RGBAPixel> const& src = surface.as_pixelview<RGBAPixel>();

  for (int y = 0; y < dst.get_height(); y += src.get_height()) {
    for (int x = 0; x < dst.get_width(); x += src.get_width()) {
      blend(pixel_blend<RGBAPixel, RGBAPixel>(), src, dst, geom::ipoint(x, y));
    }
  }

  save(SoftwareSurface(std::move(dst)), std::filesystem::path(testing::TempDir()) / "surf.PixelDataTest.blend.png");
}

TEST(PixelDataTest, empty)
{
  PixelData<RGBPixel> pixeldata;
  EXPECT_TRUE(pixeldata.empty());

  pixeldata = PixelData<RGBPixel>(geom::isize(64, 32));
  EXPECT_FALSE(pixeldata.empty());

  pixeldata = PixelData<RGBPixel>(geom::isize(0, 0));
  EXPECT_TRUE(pixeldata.empty());
}

TEST(PixelDataTest, create_view__const)
{
  PixelData<RGBPixel> const pixeldata(geom::isize(64, 32));
  std::unique_ptr<IPixelData const> pixelview = pixeldata.create_view(geom::irect(pixeldata.get_size()));
  // This shall not compile:
  // fill(*pixelview, RGBPixel{0, 0, 0});
}

TEST(PixelDataTest, move_leaves_empty)
{
  PixelData<RGB8Pixel> a({4, 4}, RGB8Pixel{1, 2, 3});
  PixelData<RGB8Pixel> b(std::move(a));
  EXPECT_EQ(b.get_size(), geom::isize(4, 4));
  EXPECT_TRUE(a.empty()); // NOLINT(bugprone-use-after-move)
  EXPECT_EQ(a.get_size(), geom::isize(0, 0)); // NOLINT(bugprone-use-after-move)

  PixelData<RGB8Pixel> c;
  c = std::move(b);
  EXPECT_EQ(c.get_size(), geom::isize(4, 4));
  EXPECT_TRUE(b.empty()); // NOLINT(bugprone-use-after-move)
}

TEST(PixelDataTest, vector_too_small)
{
  EXPECT_THROW(PixelData<RGB8Pixel>(geom::isize(4, 4), std::vector<RGB8Pixel>(15)), std::invalid_argument);
  EXPECT_THROW(PixelData<RGB8Pixel>(geom::isize(4, 4), std::vector<RGB8Pixel>(16), 5), std::invalid_argument);
  EXPECT_NO_THROW(PixelData<RGB8Pixel>(geom::isize(4, 4), std::vector<RGB8Pixel>(19), 5));
}

TEST(PixelDataTest, create_view__out_of_bounds)
{
  PixelData<RGB8Pixel> img({4, 4});
  EXPECT_THROW(img.create_view(geom::irect(2, 2, 6, 6)), std::invalid_argument);
  EXPECT_THROW(img.get_view(geom::irect(-1, 0, 2, 2)), std::invalid_argument);
}

TEST(PixelDataTest, fill_variants)
{
  for (int width : {1, 7, 8, 13, 16}) {
    // one extra column on the right that must not be touched
    PixelData<RGB8Pixel> img({width + 1, 2}, RGB8Pixel{0, 0, 0});
    PixelView<RGB8Pixel> view = img.get_view(geom::irect(0, 0, width, 2));
    detail::fill__fast(view, RGB8Pixel{1, 2, 3});
    for (int y = 0; y < 2; ++y) {
      for (int x = 0; x < width; ++x) {
        EXPECT_EQ(img.get_pixel({x, y}), (RGB8Pixel{1, 2, 3}));
      }
      EXPECT_EQ(img.get_pixel({width, y}), (RGB8Pixel{0, 0, 0}));
    }
  }
}

TEST(PixelDataTest, fill_checkerboard_clipped)
{
  PixelData<RGB8Pixel> img({4, 4}, RGB8Pixel{0, 0, 0});
  fill_checkerboard(img, geom::isize(1, 1), RGB8Pixel{1, 1, 1}, geom::irect(-10, -10, 10, 10));
  EXPECT_EQ(img.get_pixel({0, 0}), (RGB8Pixel{1, 1, 1}));
  EXPECT_EQ(img.get_pixel({1, 0}), (RGB8Pixel{0, 0, 0}));
}

TEST(PixelDataTest, copy_from_subview)
{
  PixelData<L8Pixel> img({8, 8});
  img.put_pixel({7, 7}, L8Pixel{42});
  PixelData<L8Pixel> const copy(img.get_view(geom::irect(4, 4, 8, 8)));
  EXPECT_EQ(copy.get_size(), geom::isize(4, 4));
  EXPECT_EQ(copy.get_pitch(), 4);
  EXPECT_EQ(copy.get_pixel({3, 3}).l, 42);
}

/* EOF */
