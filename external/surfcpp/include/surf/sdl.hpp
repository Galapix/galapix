// surf - Software surface library
// Copyright (C) 2008-2020 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or (at your
// option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
// License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#ifndef HEADER_SURF_SDL_HPP
#define HEADER_SURF_SDL_HPP

#include <SDL.h>
#include <assert.h>
#include <string.h>

#include <format>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <utility>

#include "fwd.hpp"
#include "software_surface.hpp"

namespace surf {

class SDLSurfacePtr
{
public:
  SDLSurfacePtr() : m_surf(nullptr) {}
  SDLSurfacePtr(SDL_Surface* surf) : m_surf(surf) { assert(m_surf != nullptr); }
  SDLSurfacePtr(SDLSurfacePtr&& other) noexcept : m_surf(std::exchange(other.m_surf, nullptr)) {}
  SDLSurfacePtr(const SDLSurfacePtr&) = delete;
  SDLSurfacePtr& operator=(const SDLSurfacePtr&) = delete;
  ~SDLSurfacePtr() { destroy(); }

  SDLSurfacePtr& operator=(SDLSurfacePtr&& other) noexcept {
    if (this == &other) { return *this; }
    destroy();
    m_surf = std::exchange(other.m_surf, nullptr);
    return *this;
  }

  void reset(SDL_Surface* surf) { destroy(); m_surf = surf; }
  SDL_Surface& operator*() const { return *m_surf; }
  SDL_Surface* get() const { return m_surf; }
  SDL_Surface* operator->() const { return m_surf; }

private:
  void destroy() {
    if (m_surf == nullptr) { return; }

    SDL_FreeSurface(m_surf);
    m_surf = nullptr;
  }

private:
  SDL_Surface* m_surf;
};

/** Create an SDL_Surface from PixelData without copying it */
template<typename Pixel>
SDLSurfacePtr create_sdl_surface_view(PixelData<Pixel>& pixeldata)
{
  SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(pixeldata.get_data(),
                                                  pixeldata.get_width(),
                                                  pixeldata.get_height(),
                                                  PPixelFormat<Pixel>::bits_per_pixel,
                                                  pixeldata.get_pitch(),
                                                  PPixelFormat<Pixel>::rmask,
                                                  PPixelFormat<Pixel>::gmask,
                                                  PPixelFormat<Pixel>::bmask,
                                                  PPixelFormat<Pixel>::amask);
  if (surface == nullptr) {
    std::ostringstream oss;
    oss << "SDL_Surface creation failed: " << SDL_GetError();
    throw std::runtime_error(oss.str());
  }

  return SDLSurfacePtr(surface);
}

/** Create an SDL_Surface from PixelData without copying it. */
template<typename Pixel>
SDLSurfacePtr create_sdl_surface_view(PixelData<Pixel> const& pixeldata)
{
  return create_sdl_surface_view(const_cast<PixelData<Pixel>&>(pixeldata));
}

/** Create an SDL_Surface and copy PixelData into it. */
template<typename Pixel>
SDLSurfacePtr create_sdl_surface(PixelData<Pixel> const& pixeldata)
{
  SDL_Surface* surface = SDL_CreateRGBSurface(0,
                                              pixeldata.get_width(),
                                              pixeldata.get_height(),
                                              PPixelFormat<Pixel>::bits_per_pixel,
                                              PPixelFormat<Pixel>::rmask,
                                              PPixelFormat<Pixel>::gmask,
                                              PPixelFormat<Pixel>::bmask,
                                              PPixelFormat<Pixel>::amask);
  if (surface == nullptr) {
    std::ostringstream oss;
    oss << "SDL_Surface creation failed: " << SDL_GetError();
    throw std::runtime_error(oss.str());
  }

  SDL_LockSurface(surface);
  for (int y = 0; y < pixeldata.get_height(); ++y) {
    memcpy(&static_cast<uint8_t*>(surface->pixels)[y * surface->pitch],
           pixeldata.get_row(y),
           pixeldata.get_width() * sizeof(Pixel));
  }
  SDL_UnlockSurface(surface);

  return surface;
}

template<typename Pixel>
PixelData<Pixel> pixeldata_from_sdl_surface(SDL_Surface& surface)
{
  PixelData<Pixel> pixeldata({surface.w, surface.h});
  SDLSurfacePtr dst(create_sdl_surface_view(pixeldata));
  if (SDL_BlitSurface(&surface, nullptr, dst.get(), nullptr) != 0) {
    std::ostringstream oss;
    oss << "from_sdl_surface() failed:: " << SDL_GetError();
    throw std::runtime_error(oss.str());
  }
  return pixeldata;
}

inline
std::unique_ptr<IPixelData> pixelview_from_sdl_surface(SDL_Surface& surface)
{
  switch (surface.format->format)
  {
    case SDL_PIXELFORMAT_RGB24:
      return std::make_unique<PixelView<RGBPixel>>(
        PixelView<RGBPixel>::from_pitch(geom::isize(surface.w, surface.h),
                                        static_cast<RGBPixel*>(surface.pixels),
                                        surface.pitch));

    case SDL_PIXELFORMAT_RGBA32:
      return std::make_unique<PixelView<RGBAPixel>>(
        PixelView<RGBAPixel>::from_pitch(geom::isize(surface.w, surface.h),
                                         static_cast<RGBAPixel*>(surface.pixels),
                                         surface.pitch));

    default:
      throw std::runtime_error(std::format("unsupported SDL_PixelFormatEnum: {}", surface.format->format));
  }
}

inline
SoftwareSurface softwaresurface_from_sdl_surface(SDL_Surface& surface)
{
  switch (surface.format->format)
  {
    case SDL_PIXELFORMAT_RGB24:
      return SoftwareSurface(pixeldata_from_sdl_surface<RGBPixel>(surface));

    case SDL_PIXELFORMAT_RGBA32:
      return SoftwareSurface(pixeldata_from_sdl_surface<RGBAPixel>(surface));

    default:
      throw std::runtime_error(std::format("unsupported SDL_PixelFormatEnum: {}", surface.format->format));
  }
}

inline
SoftwareSurface softwaresurface_view_from_sdl_surface(SDL_Surface& surface)
{
  return SoftwareSurface(pixelview_from_sdl_surface(surface));
}

} // namespace surf

#endif

/* EOF */
