#include <iostream>
#include <sstream>
#include <gtest/gtest.h>

#include <geom/rect.hpp>
#include <geom/io.hpp>

#include <surf/pixel_data.hpp>
#include <surf/sdl.hpp>
#include <surf/transform.hpp>

using namespace surf;

TEST(SDLTest, create_sdl_surface_view)
{
  PixelData<RGBPixel> const red(geom::isize{8, 16}, RGBPixel{255, 0, 0});
  PixelData<RGBPixel> const white(geom::isize{8, 16}, RGBPixel{255, 255, 255});
  PixelData<RGBPixel> const pixeldata = white; // NOLINT

  SDLSurfacePtr sdl_surf = create_sdl_surface_view(pixeldata);

  EXPECT_NE(sdl_surf.get(), nullptr);

  EXPECT_EQ(white, pixeldata);

  SDL_FillRect(sdl_surf.get(), nullptr, SDL_MapRGB(sdl_surf->format, 255, 0, 0));

  EXPECT_EQ(red, pixeldata);
}

TEST(SDLTest, create_sdl_surface)
{
  PixelData<RGBPixel> pixeldata({256, 128});
  SDLSurfacePtr sdl_surf = create_sdl_surface_view(pixeldata);
}

TEST(SDLTest, from_sdl_surface)
{
  PixelData<RGBPixel> const red(geom::isize{8, 16}, RGBPixel{255, 0, 0});
  SDLSurfacePtr sdl_red = create_sdl_surface(red);
  PixelData<RGBPixel> const red_out = pixeldata_from_sdl_surface<RGBPixel>(*sdl_red);
  EXPECT_EQ(red, red);
  EXPECT_EQ(red_out, red_out);
  EXPECT_EQ(red, red_out);
}

TEST(SDLTest, view_padded_rgb24)
{
  // SDL pads RGB24 rows to four bytes, 5 * 3 = 15 -> pitch of 16
  SDLSurfacePtr sdl_surf(SDL_CreateRGBSurfaceWithFormat(0, 5, 3, 24, SDL_PIXELFORMAT_RGB24));
  ASSERT_NE(sdl_surf->pitch, 15);

  uint8_t* const pixels = static_cast<uint8_t*>(sdl_surf->pixels);
  for (int y = 0; y < 3; ++y) {
    for (int x = 0; x < 5; ++x) {
      uint8_t* const px = pixels + y * sdl_surf->pitch + x * 3;
      px[0] = static_cast<uint8_t>(x);
      px[1] = static_cast<uint8_t>(y);
      px[2] = 0xff;
    }
  }

  SoftwareSurface const surface = softwaresurface_view_from_sdl_surface(*sdl_surf);
  EXPECT_EQ(surface.get_pitch(), sdl_surf->pitch);
  PixelView<RGB8Pixel> const& view = surface.as_pixelview<RGB8Pixel>();
  EXPECT_EQ(view.get_pixel({0, 0}), (RGB8Pixel{0, 0, 0xff}));
  EXPECT_EQ(view.get_pixel({4, 1}), (RGB8Pixel{4, 1, 0xff}));
  EXPECT_EQ(view.get_pixel({4, 2}), (RGB8Pixel{4, 2, 0xff}));

  // copying out of the view produces a tightly packed PixelData
  PixelData<RGB8Pixel> const copy(view);
  EXPECT_EQ(copy.get_pitch(), 15);
  EXPECT_EQ(copy.get_pixel({4, 2}), (RGB8Pixel{4, 2, 0xff}));
}

/* EOF */
