// SPDX-FileCopyrightText: 2005-2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <wstdisplay/font/text_area.hpp>

#include <cmath>
#include <vector>

#include <babyxml.hpp>
#include <surf/color.hpp>

#include <wstdisplay/canvas.hpp>
#include <wstdisplay/font/font.hpp>

namespace wstdisplay {

namespace {

struct TextAreaCommand
{
  enum Type { WORD, START, END } type;
  std::string content;
};

} // namespace

class TextAreaImpl
{
public:
  Font const* font = nullptr;
  geom::frect rect = {};
  float passed_time = 0.0f;
  float v_space = 2.0f;
  bool letter_by_letter = false;
  bool progress_complete = false;
  std::vector<TextAreaCommand> commands = {};
  glm::vec2 cursor_pos = {};
  float scroll_offset = 0.0f;
  float max_scroll_offset = -1.0f;
};

TextArea::TextArea(Font const* font, geom::frect const& rect, bool letter_by_letter) :
  impl(std::make_unique<TextAreaImpl>())
{
  impl->font = font;
  impl->rect = rect;
  impl->letter_by_letter = letter_by_letter;
}

TextArea::~TextArea()
{
}

void
TextArea::set_rect(geom::frect const& rect)
{
  impl->rect = rect;
  impl->max_scroll_offset = -1.0f;
}

geom::frect
TextArea::get_rect() const
{
  return impl->rect;
}

void
TextArea::set_text(std::string const& str)
{
  impl->scroll_offset = 0.0f;
  impl->max_scroll_offset = -1.0f;
  impl->progress_complete = false;
  impl->commands.clear();

  for (auto const& node : babyxml::parse(str))
  {
    if (node.type == babyxml::NodeType::START_TAG) {
      impl->commands.push_back({TextAreaCommand::START, node.content});
    } else if (node.type == babyxml::NodeType::END_TAG) {
      impl->commands.push_back({TextAreaCommand::END, node.content});
    } else if (node.type == babyxml::NodeType::TEXT) {
      // split into words, ' ' and '\n' are words of their own:
      // "Hello  World \n" => ("Hello", " ", " ", "World", " ", "\n")
      std::string word;
      for (char const c : node.content) {
        if (c == ' ' || c == '\n') {
          if (!word.empty()) {
            impl->commands.push_back({TextAreaCommand::WORD, word});
            word.clear();
          }
          impl->commands.push_back({TextAreaCommand::WORD, std::string(1, c)});
        } else {
          word += c;
        }
      }
      if (!word.empty()) {
        impl->commands.push_back({TextAreaCommand::WORD, word});
      }
    }
  }
}

void
TextArea::set_font(Font const* font)
{
  impl->font = font;
  impl->max_scroll_offset = -1.0f;
}

void
TextArea::set_progress_complete()
{
  impl->letter_by_letter = false;
}

bool
TextArea::is_progress_complete() const
{
  return impl->progress_complete;
}

void
TextArea::update(float delta)
{
  impl->passed_time += delta;
}

void
TextArea::draw(Canvas& canvas)
{
  Font const& font = *impl->font;
  float const line_height = font.get_line_height() + impl->v_space;

  if (impl->max_scroll_offset > 0.0f)
  {
    float const height = impl->max_scroll_offset + impl->rect.height();
    canvas.fill_rounded_rect(geom::frect(geom::fpoint(impl->rect.right() + 4,
                                                      impl->rect.top() + impl->scroll_offset * impl->rect.height() / height),
                                         geom::fsize(8, impl->rect.height() * impl->rect.height() / height)),
                             4.0f, surf::Color(1.0f, 1.0f, 1.0f, 0.25f));
  }

  Canvas::Scope scope(canvas);
  canvas.translate(impl->rect.left(), impl->rect.top() + font.get_height() - impl->scroll_offset);

  surf::Color top_color(1.0f, 1.0f, 1.0f);
  surf::Color bottom_color(1.0f, 1.0f, 1.0f);
  float scale = 1.0f;
  bool sinus = false;
  float eat_time = impl->passed_time;
  bool break_writing = false;

  float x_pos = 0.0f;
  float y_pos = 0.0f;

  auto it = impl->commands.begin();
  for (; it != impl->commands.end() && !break_writing; ++it)
  {
    switch (it->type)
    {
      case TextAreaCommand::START:
        if (it->content == "b") {
          top_color = surf::Color(1.0f, 0.0f, 0.0f);
          bottom_color = surf::Color(0.8f, 0.0f, 0.0f);
        } else if (it->content == "i") {
          top_color = bottom_color = surf::Color(0.65f, 0.7f, 1.0f);
        } else if (it->content == "small") {
          scale = 0.6f;
        } else if (it->content == "large") {
          scale = 2.0f;
        } else if (it->content == "sleep") {
          eat_time -= 1.0f;
        } else if (it->content == "sin") {
          sinus = true;
        }
        break;

      case TextAreaCommand::END:
        if (it->content == "b" || it->content == "i") {
          top_color = bottom_color = surf::Color(1.0f, 1.0f, 1.0f);
        } else if (it->content == "small" || it->content == "large") {
          scale = 1.0f;
        } else if (it->content == "sin") {
          sinus = false;
        }
        break;

      case TextAreaCommand::WORD:
      {
        float const word_width = font.get_width(it->content) * scale;

        if (it->content == "\n") {
          x_pos = 0.0f;
          y_pos += line_height;
          break;
        }

        if (x_pos + word_width > impl->rect.width() && word_width <= impl->rect.width()) {
          x_pos = 0.0f;
          y_pos += line_height;
        }

        if (x_pos == 0.0f && it->content == " ") {
          break; // no space at the beginning of a line
        }

        std::string_view const word = it->content;
        for (size_t pos = 0; pos < word.size();)
        {
          if (impl->letter_by_letter && eat_time <= 0.0f) {
            break_writing = true;
            break;
          }

          char32_t const cp = utf8_next(word, pos);
          eat_time -= (cp == U'.') ? 0.5f : 0.05f;

          Glyph const* glyph = font.get_glyph_or_fallback(cp);
          if (!glyph) {
            continue;
          }

          float y = y_pos;
          if (sinus) {
            y += std::sin(impl->passed_time * 10.0f + x_pos / 15.0f) * 5.0f;
          }

          bool const visible = (y_pos >= impl->scroll_offset &&
                                y_pos < impl->scroll_offset + impl->rect.height() - font.get_height());
          if (visible) {
            Font::draw_glyph(canvas, *glyph, geom::fpoint(x_pos, y), scale, top_color, bottom_color);
          }

          x_pos += glyph->advance * scale;
        }
        break;
      }
    }
  }

  if (impl->max_scroll_offset == -1.0f) {
    impl->max_scroll_offset = std::max(0.0f, y_pos - impl->rect.height());
  }

  if (it == impl->commands.end() && !break_writing) {
    impl->progress_complete = true;
  }

  impl->cursor_pos = glm::vec2(x_pos + impl->rect.left(), y_pos + impl->rect.top());
}

glm::vec2
TextArea::get_cursor_pos() const
{
  return impl->cursor_pos;
}

float
TextArea::get_scroll_offset() const
{
  return impl->scroll_offset;
}

void
TextArea::set_scroll_offset(float s)
{
  impl->scroll_offset = std::clamp(s, 0.0f, std::max(0.0f, impl->max_scroll_offset));
}

} // namespace wstdisplay

/* EOF */
