// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef HEADER_WSTSPRITE_PROPERTIES_HPP
#define HEADER_WSTSPRITE_PROPERTIES_HPP

#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <glm/glm.hpp>

namespace wstsprite {

/** Values of keys the library doesn't interpret, kept as strings so
    games can read their own extensions (hitboxes, sounds, ...) */
class Properties
{
public:
  bool has(std::string_view key) const;

  std::optional<std::string> get_string(std::string_view key) const;
  std::optional<float> get_float(std::string_view key) const;
  std::optional<int> get_int(std::string_view key) const;
  std::optional<bool> get_bool(std::string_view key) const;
  std::optional<glm::vec2> get_vec2(std::string_view key) const;

  /** All values of \a key, e.g. the four numbers of a hitbox */
  std::vector<std::string> const* get(std::string_view key) const;

  void set(std::string key, std::vector<std::string> values);

  std::map<std::string, std::vector<std::string>, std::less<>> const& get_all() const { return m_values; }

private:
  std::map<std::string, std::vector<std::string>, std::less<>> m_values;
};

} // namespace wstsprite

#endif

/* EOF */
