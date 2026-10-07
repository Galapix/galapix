// Galapix - an image viewer for large image collections
// Copyright (C) 2026 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#ifndef HEADER_GALAPIX_GALAPIX_COMPONENTS_HPP
#define HEADER_GALAPIX_GALAPIX_COMPONENTS_HPP

#include <memory>

#include "galapix/image_tiles.hpp"
#include "galapix/tile_provider.hpp"
#include "math/rect.hpp"
#include "math/size.hpp"
#include "math/vector2f.hpp"
#include "util/url.hpp"

/** Components of a workspace image entity (see Workspace). Plain data;
    the functions below compute derived geometry. */
namespace galapix {

/** Where the image comes from */
struct Source
{
  URL url = {};
};

/** Placement in the workspace: center position, scale of the native
    pixels and rotation in degrees */
struct Transform
{
  Vector2f pos = Vector2f(0.0f, 0.0f);
  float scale = 1.0f;
  float angle = 0.0f;
};

/** Native size in pixels; 256x256 while the size probe is pending */
struct ImageSize
{
  int width = 256;
  int height = 256;
};

/** Tile source and on-screen tiles, absent while the size is unknown */
struct Tiles
{
  TileProviderPtr provider = {};
  std::unique_ptr<ImageTiles> tiles = {};
};

/** Tag: inside the view rect this frame */
struct Visible {};

/** Tag: part of the selection */
struct Selected {};

/** Tag: Transform::scale refers to the native size already (loaded from a
    workspace file), set_tile_provider() must not rescale it */
struct ScaleIsNative {};

inline Sizef scaled_size(Transform const& transform, ImageSize const& size)
{
  return Sizef(static_cast<float>(size.width) * transform.scale,
               static_cast<float>(size.height) * transform.scale);
}

inline Vector2f top_left(Transform const& transform, ImageSize const& size)
{
  Sizef const s = scaled_size(transform, size);
  return transform.pos - geom::fsize(s.width() / 2.0f, s.height() / 2.0f);
}

inline void set_top_left(Transform& transform, ImageSize const& size, Vector2f const& pos)
{
  Sizef const s = scaled_size(transform, size);
  transform.pos = pos + geom::fsize(s.width() / 2.0f, s.height() / 2.0f);
}

inline Rectf image_rect(Transform const& transform, ImageSize const& size)
{
  return Rectf(top_left(transform, size), scaled_size(transform, size));
}

} // namespace galapix

#endif

/* EOF */
