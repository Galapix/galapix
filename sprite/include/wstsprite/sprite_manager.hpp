// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef HEADER_WSTSPRITE_SPRITE_MANAGER_HPP
#define HEADER_WSTSPRITE_SPRITE_MANAGER_HPP

#include <filesystem>
#include <map>

#include <wstdisplay/fwd.hpp>
#include <wstdisplay/resource_pool.hpp>

#include "sprite.hpp"
#include "sprite_data.hpp"

namespace wstsprite {

/** Owns SpriteData and refers to it by SpriteId. Files are loaded
    once and cached by filename, plain images are accepted as single
    frame sprites. Frame events are resolved with \a events if given,
    which must outlive the manager. */
class SpriteManager final
{
public:
  explicit SpriteManager(wstdisplay::SurfaceManager& surfaces, EventRegistry const* events = nullptr);

  /** Load \a filename, or return the id it was loaded with before.
      Throws when the file can't be loaded. */
  SpriteId load(std::filesystem::path const& filename);

  /** Take ownership of data not loaded from a file, e.g. from
      SpriteData::from_mapping() */
  SpriteId add(SpriteData data);

  /** The data of \a id, nullptr if it is null or was unloaded */
  SpriteData const* find(SpriteId id) const { return m_pool.find(id); }

  /** The data of \a id, throws if it is null or was unloaded */
  SpriteData const& get(SpriteId id) const;

  /** Delete the data, \a id and its copies become stale */
  void unload(SpriteId id);

  /** Unload everything */
  void clear();

  size_t count() const { return m_pool.count(); }

  /** A Sprite for \a filename, see load() */
  Sprite create(std::filesystem::path const& filename);

private:
  wstdisplay::SurfaceManager& m_surfaces;
  EventRegistry const* m_events;
  wstdisplay::ResourcePool<SpriteData> m_pool;
  std::map<std::filesystem::path, SpriteId> m_files;

public:
  SpriteManager(SpriteManager const&) = delete;
  SpriteManager& operator=(SpriteManager const&) = delete;
};

} // namespace wstsprite

#endif

/* EOF */
