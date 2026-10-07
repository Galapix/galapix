// SPDX-FileCopyrightText: 2002-2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef HEADER_WSTDISPLAY_SURFACE_MANAGER_HPP
#define HEADER_WSTDISPLAY_SURFACE_MANAGER_HPP

#include <filesystem>
#include <map>
#include <memory>
#include <vector>

#include <geom/size.hpp>
#include <surf/software_surface.hpp>

#include "device.hpp"
#include "fwd.hpp"
#include "surface.hpp"
#include "texture_packer.hpp"
#include "texture_params.hpp"

namespace wstdisplay {

/** Loads images as Surfaces and caches them by filename. Images up
    to a size limit are packed onto shared atlas pages, larger ones
    get a texture of their own. Owns all textures it creates, the
    surfaces stay valid for the lifetime of the manager. */
class SurfaceManager final
{
public:
  struct Options
  {
    /** Size of an atlas page, clamped to GL_MAX_TEXTURE_SIZE */
    geom::isize page_size = geom::isize(2048, 2048);

    /** Images with a larger width or height get their own texture */
    int max_atlas_image_size = 512;

    /** Filtering of all textures, use Nearest for pixel art */
    TextureParams params = {};
  };

public:
  explicit SurfaceManager(Device& device);
  SurfaceManager(Device& device, Options const& options);
  ~SurfaceManager();

  /** The surface for the image \a filename, loaded on first use.
      Throws if the image can't be loaded and no fallback is set. */
  Surface get(std::filesystem::path const& filename);

  /** Returned by get() for images that can't be loaded, the error is
      logged. A null surface (the default) makes get() throw instead. */
  void set_fallback(Surface const& fallback) { m_fallback = fallback; }

  /** Upload \a image, not cached */
  Surface create(surf::SoftwareSurface const& image);

  /** Load an image and split it into cells of \a size, row by row */
  std::vector<Surface> load_grid(std::filesystem::path const& filename, geom::isize const& size);

  TexturePacker& get_packer() { return m_packer; }

private:
  Device& m_device;
  Options m_options;
  TexturePacker m_packer;

  /** Images too large for the atlas */
  std::vector<Unique<Texture>> m_textures;

  std::map<std::filesystem::path, Surface> m_surfaces;
  Surface m_fallback;

public:
  SurfaceManager(SurfaceManager const&) = delete;
  SurfaceManager& operator=(SurfaceManager const&) = delete;
};

} // namespace wstdisplay

#endif

/* EOF */
