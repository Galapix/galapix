// surf - Software surface library
// Copyright (C) 2008-2020 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or (at your
// option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
// License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#ifndef HEADER_SURF_SOFTWARE_SURFACE_LOADER_HPP
#define HEADER_SURF_SOFTWARE_SURFACE_LOADER_HPP

#include <filesystem>
#include <span>
#include <string>

#include "pixel_data.hpp"
#include "software_surface.hpp"

namespace surf {

class SoftwareSurfaceFactory;
class SoftwareSurface;

class SoftwareSurfaceLoader
{
public:
  virtual ~SoftwareSurfaceLoader() {}
  virtual std::string get_name() const = 0;

  virtual bool supports_from_file() const = 0;
  virtual bool supports_from_mem() const = 0;

  virtual SoftwareSurface from_file(std::filesystem::path const& filename) const = 0;
  virtual SoftwareSurface from_mem(std::span<uint8_t const> data) const = 0;
};

template<typename FromFileFunc, typename FromMemFunc>
class SoftwareSurfaceLoaderGeneric : public SoftwareSurfaceLoader
{
public:
  SoftwareSurfaceLoaderGeneric(std::string name, FromFileFunc from_file_func, FromMemFunc from_mem_func) :
    m_name(std::move(name)),
    m_from_file(from_file_func),
    m_from_mem(from_mem_func)
  {}
  ~SoftwareSurfaceLoaderGeneric() override {}

  std::string get_name() const override { return m_name; }

  bool supports_from_file() const override { return !std::is_null_pointer<FromFileFunc>::value; }
  bool supports_from_mem() const override { return !std::is_null_pointer<FromMemFunc>::value; }

 SoftwareSurface from_file(std::filesystem::path const& filename) const override {
   if constexpr (std::is_null_pointer<FromFileFunc>::value) {
     return {};
   } else {
     return m_from_file(filename);
   }
 }

  SoftwareSurface from_mem(std::span<uint8_t const> data) const override {
    if constexpr (std::is_null_pointer<FromMemFunc>::value) {
      return {};
    } else {
      return m_from_mem(data);
    }
  }

private:
  std::string m_name;
  FromFileFunc m_from_file;
  FromMemFunc m_from_mem;

public:
  SoftwareSurfaceLoaderGeneric(const SoftwareSurfaceLoaderGeneric&) = delete;
  SoftwareSurfaceLoaderGeneric& operator=(const SoftwareSurfaceLoaderGeneric&) = delete;
};

template<typename FromFileFunc, typename FromMemFunc> inline
std::unique_ptr<SoftwareSurfaceLoader> make_loader(std::string name, FromFileFunc from_file, FromMemFunc&& from_mem) // NOLINT
{
  return std::make_unique<SoftwareSurfaceLoaderGeneric<FromFileFunc, FromMemFunc>>(
    std::move(name),
    std::forward<FromFileFunc>(from_file),
    std::forward<FromMemFunc>(from_mem));
}

} // namespace surf

#endif

/* EOF */
