// SPDX-FileCopyrightText: 2021 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef HEADER_WSTGUI_RFLOAT_HPP
#define HEADER_WSTGUI_RFLOAT_HPP

namespace wstgui {

/** A float value that is either absolute or relative to some parent size */
class rfloat
{
private:
  rfloat(float value, bool absolute) :
    m_value(value),
    m_absolute(absolute)
  {}

public:
  static rfloat absolute(float value) { return rfloat(value, true); }
  static rfloat relative(float value) { return rfloat(value, false); }

  float resolve(float parent) const
  {
    if (m_absolute) {
      return m_value;
    } else {
      return parent * m_value;
    }
  }

  float operator()(float parent) const {
    return resolve(parent);
  }

  bool is_absolute() const { return m_absolute; }
  bool is_relative() const { return !m_absolute; }

  float raw() const { return m_value; }

private:
  float m_value;
  bool m_absolute;
};

} // namespace wstgui

#endif

/* EOF */
