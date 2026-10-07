// SPDX-FileCopyrightText: 2002-2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef HEADER_WSTDISPLAY_TEXTURE_MANAGER_HPP
#define HEADER_WSTDISPLAY_TEXTURE_MANAGER_HPP

#include <filesystem>
#include <map>
#include <utility>

#include "device.hpp"
#include "fwd.hpp"
#include "surface.hpp"
#include "texture_params.hpp"

namespace wstdisplay {

/** Loads images into textures of their own and caches them, for
    textures that can't live on an atlas, e.g. repeating patterns or
    textures used by custom shaders. Owns all textures it loads. */
class TextureManager final
{
public:
  explicit TextureManager(Device& device);
  ~TextureManager();

  /** Load \a filename, on failure the fallback is returned if one is
      set, otherwise the error is rethrown. The surface covers the
      whole texture, use Surface::get_texture() for materials. */
  Surface get(std::filesystem::path const& filename, TextureParams const& params = {});

  /** Returned by get() for images that can't be loaded */
  void set_fallback(std::filesystem::path const& filename);

private:
  using Key = std::pair<std::filesystem::path, int>;

  struct Entry
  {
    Unique<Texture> texture = {};
    Surface surface = {};
  };

  Device& m_device;
  std::map<Key, Entry> m_textures;
  Entry m_fallback;

public:
  TextureManager(TextureManager const&) = delete;
  TextureManager& operator=(TextureManager const&) = delete;
};

} // namespace wstdisplay

#endif

/* EOF */
