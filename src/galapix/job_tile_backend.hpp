// Galapix - an image viewer for large image collections
// Copyright (C) 2026 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#ifndef HEADER_GALAPIX_GALAPIX_JOB_TILE_BACKEND_HPP
#define HEADER_GALAPIX_GALAPIX_JOB_TILE_BACKEND_HPP

#include <functional>
#include <memory>

#include <thumtoo/lod/tile_backend.hpp>

#include "galapix/tile.hpp"
#include "job/job.hpp"
#include "job/job_handle.hpp"

namespace galapix {

class JobManager;

/** thumtoo::lod::TileBackend running one Job per cell on the JobManager
    (Zoomify, Mandelbrot). Every cell gets exactly one result: the tile
    the job delivers, Cancelled when the job was aborted, Failed when it
    finished without a tile. Cells outside [min_scale, max_scale] are
    Unavailable. */
class JobTileBackend final : public thumtoo::lod::TileBackend
{
public:
  using MakeJob = std::function<JobPtr(JobHandle const& handle, int scale, Vector2i const& pos,
                                       std::function<void (Tile)> const& callback)>;

  JobTileBackend(JobManager& job_manager, int min_scale, int max_scale, MakeJob make_job);
  ~JobTileBackend() override;

  void fetch(std::vector<thumtoo::lod::FetchRequest> cells, ResultFn on_result) override;
  void cancel(std::vector<thumtoo::lod::TileKey> const& keys) override;

private:
  struct Shared;

  JobManager& m_job_manager;
  int m_min_scale;
  int m_max_scale;
  MakeJob m_make_job;
  std::shared_ptr<Shared> m_shared;

public:
  JobTileBackend(JobTileBackend const&) = delete;
  JobTileBackend& operator=(JobTileBackend const&) = delete;
};

/** Convert a decoded tile to the RGBA8 bitmap thumtoo::lod stores. */
thumtoo::lod::TileBitmap tile_bitmap_from_surface(surf::SoftwareSurface const& surface);

} // namespace galapix

#endif

/* EOF */
