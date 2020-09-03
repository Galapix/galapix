// Galapix - an image viewer for large image collections
// Copyright (C) 2008-2019 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef HEADER_GALAPIX_UTIL_SHA1_HPP
#define HEADER_GALAPIX_UTIL_SHA1_HPP

#include <array>
#include <string>
#include <fmt/format.h>
#include <sstream>

#include "util/blob.hpp"

class SHA1
{
public:
  static SHA1 from_mem(std::span<uint8_t const> data);
  static SHA1 from_mem(const std::string& str);
  static SHA1 from_file(const std::string& filename);
  static SHA1 from_string(const std::string& str);

public:
  SHA1();
  SHA1(const uint8_t text[20]);
  SHA1(Blob const& blob);

  std::string str() const;
  const uint8_t* data() const { return m_data.data(); }
  size_t size() const { return m_data.size(); }

  explicit operator bool() const;

  bool operator==(const SHA1& rhs) const;
  bool operator!=(const SHA1& rhs) const;

private:
  std::array<uint8_t, 20> m_data;
};

std::ostream& operator<<(std::ostream& os, const SHA1& sha1);

template<>
struct fmt::formatter<SHA1>
{
  template<typename ParseContext>
  constexpr auto parse(ParseContext& ctx)
  {
    return ctx.begin();
  }

  template<typename FormatContext>
  auto format(SHA1 const& v, FormatContext& ctx)
  {
    std::ostringstream os;
    os << v;
    return fmt::format_to(ctx.out(), os.str());
  }
};

#endif

/* EOF */
