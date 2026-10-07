// SPDX-FileCopyrightText: 2021 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <wstgui/layout_builder.hpp>

#include <logmich/log.hpp>

#include <wstgui/cut_layoutable.hpp>

namespace wstgui {

LayoutBuilder::LayoutBuilder() :
  m_stack(),
  m_root()
{
}

LayoutBuilder&
LayoutBuilder::begin_hcut(float length)
{
  m_stack.emplace_back(std::make_unique<CutLayoutable>(CutLayoutable::Orientation::Horizontal, length));
  return *this;
}

LayoutBuilder&
LayoutBuilder::begin_vcut(float length)
{
  m_stack.emplace_back(std::make_unique<CutLayoutable>(CutLayoutable::Orientation::Vertical, length));
  return *this;
}

LayoutBuilder&
LayoutBuilder::next()
{
  log_not_implemented();

  return *this;
}

LayoutBuilder&
LayoutBuilder::pack(Component* component)
{
  pack(std::make_unique<LayoutableComponent>(component));

  return *this;
}

LayoutBuilder&
LayoutBuilder::pack(std::unique_ptr<ILayoutable> layoutable)
{
  if (m_stack.empty()) {
    throw std::runtime_error("LayoutBuilder stack empty");
  }

  m_stack.back()->pack(std::move(layoutable));

  return *this;
}

LayoutBuilder&
LayoutBuilder::end()
{
  std::unique_ptr<CutLayoutable> cutable = std::move(m_stack.back());
  m_stack.pop_back();

  if (!m_stack.empty()) {
    m_stack.back()->pack(std::move(cutable));
  } else {
    m_root = std::move(cutable);
  }

  return *this;
}

std::unique_ptr<ILayoutable>
LayoutBuilder::finalize()
{
  if (!m_root) {
    throw std::runtime_error("LayoutBuilder not finished");
  }

  return std::move(m_root);
}

} // namespace wstgui

/* EOF */
