// SPDX-FileCopyrightText: 2002-2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <wstdisplay/texture_manager.hpp>

#include <exception>

#include <logmich/log.hpp>
#include <surf/software_surface.hpp>

#include <wstdisplay/texture.hpp>

namespace wstdisplay {

TextureManager::TextureManager(Device& device) :
  m_device(device),
  m_textures(),
  m_fallback()
{
}

TextureManager::~TextureManager()
{
}

Surface
TextureManager::get(std::filesystem::path const& filename, TextureParams const& params)
{
  Key const key{filename, static_cast<int>(params.filter) |
                          (static_cast<int>(params.wrap_x) << 4) |
                          (static_cast<int>(params.wrap_y) << 8)};
  auto it = m_textures.find(key);
  if (it != m_textures.end()) {
    return it->second.surface;
  }

  try {
    surf::SoftwareSurface const image = surf::SoftwareSurface::from_file(filename);
    Unique<Texture> texture = m_device.create_texture(image, params);
    Surface const surface(texture, image.get_size());
    m_textures[key] = Entry{std::move(texture), surface};
    return surface;
  } catch (std::exception const& err) {
    if (!m_fallback.surface) {
      throw;
    }
    log_error("TextureManager: {}", err.what());
    return m_fallback.surface;
  }
}

void
TextureManager::set_fallback(std::filesystem::path const& filename)
{
  surf::SoftwareSurface const image = surf::SoftwareSurface::from_file(filename);
  Unique<Texture> texture = m_device.create_texture(image);
  Surface const surface(texture, image.get_size());
  m_fallback = Entry{std::move(texture), surface};
}

} // namespace wstdisplay

/* EOF */
