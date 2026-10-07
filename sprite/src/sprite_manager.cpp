// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <wstsprite/sprite_manager.hpp>

#include <stdexcept>

#include <wstdisplay/surface_manager.hpp>

namespace wstsprite {

SpriteManager::SpriteManager(wstdisplay::SurfaceManager& surfaces, EventRegistry const* events) :
  m_surfaces(surfaces),
  m_events(events),
  m_pool(),
  m_files()
{
}

SpriteId
SpriteManager::load(std::filesystem::path const& filename)
{
  auto it = m_files.find(filename);
  if (it != m_files.end()) {
    return it->second;
  }

  SpriteId const id = add((filename.extension() == ".sprite") ?
                          SpriteData::from_file(filename, m_surfaces, m_events) :
                          SpriteData::from_surface(m_surfaces.get(filename)));
  m_files.emplace(filename, id);
  return id;
}

SpriteId
SpriteManager::add(SpriteData data)
{
  return m_pool.insert(std::make_unique<SpriteData>(std::move(data)));
}

SpriteData const&
SpriteManager::get(SpriteId id) const
{
  SpriteData const* const data = find(id);
  if (data == nullptr) {
    throw std::invalid_argument(std::string("SpriteManager::get: ") + (id ? "stale" : "null") + " SpriteId");
  }
  return *data;
}

void
SpriteManager::unload(SpriteId id)
{
  std::erase_if(m_files, [id](auto const& item) { return item.second == id; });
  m_pool.destroy(id);
  m_pool.collect();
}

void
SpriteManager::clear()
{
  m_files.clear();
  m_pool.destroy_all();
  m_pool.collect();
}

Sprite
SpriteManager::create(std::filesystem::path const& filename)
{
  return Sprite(*this, load(filename));
}

} // namespace wstsprite

/* EOF */
