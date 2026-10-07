// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <wstdisplay/font/font_manager.hpp>

#include <map>
#include <stdexcept>
#include <string>
#include <tuple>

#include "font/ttf_font.hpp"
#include <wstdisplay/resource_pool.hpp>

namespace wstdisplay {

class FontManager::Impl final
{
public:
  using Key = std::tuple<std::filesystem::path, int, FontEffect>;

  explicit Impl(Device& device_) :
    device(device_),
    freetype(),
    pool(),
    files()
  {
    if (FT_Init_FreeType(&freetype)) {
      throw std::runtime_error("FontManager: couldn't initialize FreeType");
    }
  }

  ~Impl()
  {
    // the fonts need the library
    pool.clear();
    FT_Done_FreeType(freetype);
  }

  Device& device;
  FT_Library freetype;
  ResourcePool<Font> pool;
  std::map<Key, FontId> files;

public:
  Impl(Impl const&) = delete;
  Impl& operator=(Impl const&) = delete;
};

FontManager::FontManager(Device& device) :
  m_impl(std::make_unique<Impl>(device))
{
}

FontManager::~FontManager()
{
}

FontId
FontManager::load(std::filesystem::path const& filename, int size, FontEffect const& effect)
{
  Impl::Key key(filename, size, effect);
  auto it = m_impl->files.find(key);
  if (it != m_impl->files.end()) {
    return it->second;
  }

  FontId const id = add(std::make_unique<TTFFont>(m_impl->device, m_impl->freetype, filename, size, effect));
  m_impl->files.emplace(std::move(key), id);
  return id;
}

FontId
FontManager::add(std::unique_ptr<Font> font)
{
  if (!font) {
    throw std::invalid_argument("FontManager::add: null font");
  }
  return m_impl->pool.insert(std::move(font));
}

Font const*
FontManager::find(FontId id) const
{
  return m_impl->pool.find(id);
}

Font const&
FontManager::get(FontId id) const
{
  Font const* const font = find(id);
  if (font == nullptr) {
    throw std::invalid_argument(std::string("FontManager::get: ") + (id ? "stale" : "null") + " FontId");
  }
  return *font;
}

void
FontManager::unload(FontId id)
{
  std::erase_if(m_impl->files, [id](auto const& item) { return item.second == id; });
  m_impl->pool.destroy(id);
  m_impl->pool.collect();
}

size_t
FontManager::count() const
{
  return m_impl->pool.count();
}

} // namespace wstdisplay

/* EOF */
