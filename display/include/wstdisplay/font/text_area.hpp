// SPDX-FileCopyrightText: 2005-2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef HEADER_WSTDISPLAY_FONT_TEXT_AREA_HPP
#define HEADER_WSTDISPLAY_FONT_TEXT_AREA_HPP

#include <memory>
#include <string>

#include <glm/glm.hpp>
#include <geom/rect.hpp>

#include "../fwd.hpp"

namespace wstdisplay {

class TextAreaImpl;

/** Word wrapped text in a rectangle with simple markup and optional
    letter by letter display, e.g. for dialogs.

    Markup: <b>red</b>, <i>blue</i>, <small>, <large>, <sin>wavy</sin>
    and <sleep/> to pause the letter by letter display. */
class TextArea final
{
public:
  TextArea(Font const* font, geom::frect const& rect, bool letter_by_letter);
  ~TextArea();

  void set_rect(geom::frect const& rect);
  geom::frect get_rect() const;

  void set_text(std::string const& str);
  void set_font(Font const* font);

  /** Skips letter by letter display */
  void set_progress_complete();
  bool is_progress_complete() const;

  void update(float delta);
  void draw(Canvas& canvas);

  /** Position after the last drawn character, e.g. for a cursor */
  glm::vec2 get_cursor_pos() const;

  float get_scroll_offset() const;
  void set_scroll_offset(float s);

private:
  std::unique_ptr<TextAreaImpl> impl;

public:
  TextArea(TextArea const&) = delete;
  TextArea& operator=(TextArea const&) = delete;
};

} // namespace wstdisplay

#endif

/* EOF */
