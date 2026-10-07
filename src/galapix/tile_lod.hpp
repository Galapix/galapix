// Galapix - an image viewer for large image collections
// Copyright (C) 2026 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#ifndef HEADER_GALAPIX_GALAPIX_TILE_LOD_HPP
#define HEADER_GALAPIX_GALAPIX_TILE_LOD_HPP

#include <memory>

#include <thumtoo/lod/tile_loader.hpp>
#include <thumtoo/lod/tile_scheduler.hpp>

#include "galapix/tile_provider.hpp"

namespace galapix::tile_lod {

/** The TileLoader for \a provider: shared between all images with the
    same TileProvider::get_loader_key(), registered with the scheduler. */
std::shared_ptr<thumtoo::lod::TileLoader> loader_for(TileProvider& provider);

/** Main thread, once per frame: run TileScheduler::pump() when the
    scheduler asked for it (its wake hook only records the time and
    requests a redraw). */
void pump_if_due();

thumtoo::lod::TileScheduler::Stats scheduler_stats();

} // namespace galapix::tile_lod

#endif

/* EOF */
