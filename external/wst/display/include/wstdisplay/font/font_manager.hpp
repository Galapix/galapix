// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef HEADER_WSTDISPLAY_FONT_FONT_MANAGER_HPP
#define HEADER_WSTDISPLAY_FONT_FONT_MANAGER_HPP

#include <filesystem>
#include <memory>

#include "../fwd.hpp"
#include "font_effect.hpp"

namespace wstdisplay {

/** Owns fonts and refers to them by FontId. TrueType fonts are loaded
    with FreeType and cached by file, size and effect, other fonts such
    as BitmapFont are added. The glyph pages live on \a device. Font
    references stay valid until the font is unloaded. */
class FontManager final
{
public:
  explicit FontManager(Device& device);
  ~FontManager();

  /** Load the TrueType font \a filename at \a size pixels, or return
      the id it was loaded with before. Throws when the file can't be
      loaded. */
  FontId load(std::filesystem::path const& filename, int size, FontEffect const& effect = {});

  /** Take ownership of \a font, e.g. from BitmapFont::from_grid() */
  FontId add(std::unique_ptr<Font> font);

  /** The font of \a id, nullptr if it is null or was unloaded */
  Font const* find(FontId id) const;

  /** The font of \a id, throws if it is null or was unloaded */
  Font const& get(FontId id) const;

  /** Delete the font, \a id and its copies become stale */
  void unload(FontId id);

  size_t count() const;

private:
  class Impl;
  std::unique_ptr<Impl> m_impl;

public:
  FontManager(FontManager const&) = delete;
  FontManager& operator=(FontManager const&) = delete;
};

} // namespace wstdisplay

#endif

/* EOF */
