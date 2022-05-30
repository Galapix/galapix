// Galapix - an image viewer for large image collections
// Copyright (C) 2008-2019 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef HEADER_GALAPIX_GALAPIX_ZOOMIFY_TILE_PROVIDER_HPP
#define HEADER_GALAPIX_GALAPIX_ZOOMIFY_TILE_PROVIDER_HPP

#include "math/size.hpp"
#include "galapix/zoomify_tile_job.hpp"
#include "galapix/tile_provider.hpp"

class JobManager;

class ZoomifyTileProvider final : public TileProvider
{
private:
  struct Info
  {
    /** Size of the image in tiles */
    Size m_size;

    /** Number of tiles used by previous zoom levels */
    int m_previous_tiles_count;

    Info() :
      m_size(),
      m_previous_tiles_count()
    {}

    Info(Size const& size,
         int previous_tiles_count) :
      m_size(size),
      m_previous_tiles_count(previous_tiles_count)
    {}
  };

public:
  static std::shared_ptr<ZoomifyTileProvider> create(URL const& url, JobManager& job_manager);

private:
  ZoomifyTileProvider(std::string const& basedir, Size const& size, int tilesize, JobManager& job_manager);

public:
  JobHandle request_tile(int scale, Vector2i const& pos,
                         const std::function<void (Tile)>& callback) override;

  int  get_max_scale() const override { return m_max_scale; }
  int  get_tilesize() const override { return m_tilesize; }
  Size get_size() const override { return m_size; }

private:
  int get_tile_group(int scale, Vector2i const& pos);

private:
  Size        m_size;
  int         m_tilesize;
  std::string m_basedir;
  int         m_max_scale;
  std::vector<Info> m_info;
  JobManager& m_job_manager;

private:
  ZoomifyTileProvider(ZoomifyTileProvider const&) = delete;
  ZoomifyTileProvider& operator=(ZoomifyTileProvider const&) = delete;
};

#endif

/* EOF */
