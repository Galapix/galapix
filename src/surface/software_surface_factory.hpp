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

#ifndef HEADER_GALAPIX_UTIL_SOFTWARE_SURFACE_FACTORY_HPP
#define HEADER_GALAPIX_UTIL_SOFTWARE_SURFACE_FACTORY_HPP

#include <map>
#include <string>

#include "util/currenton.hpp"
#include "surface/software_surface.hpp"

class SoftwareSurfaceLoader;
class URL;

class SoftwareSurfaceFactory : public Currenton<SoftwareSurfaceFactory>
{
private:
  typedef std::map<std::string, const SoftwareSurfaceLoader*> ExtensionMap;
  typedef std::map<std::string, const SoftwareSurfaceLoader*> MimeTypeMap;
  typedef std::map<std::string, const SoftwareSurfaceLoader*> MagicMap;

public:
  SoftwareSurfaceFactory();
  ~SoftwareSurfaceFactory() override;

  void add_loader(std::unique_ptr<SoftwareSurfaceLoader> loader);
  bool has_supported_extension(const URL& url);

  /** Files are handled in the order of mime-type, extension, magic,
      if one fails, the next in line is tried. Extension is the most
      important one, as it is used to filter valid files when
      generating file lists */
  void register_by_magic(const SoftwareSurfaceLoader* loader, const std::string& magic);
  void register_by_mime_type(const SoftwareSurfaceLoader* loader, const std::string& mime_type);
  void register_by_extension(const SoftwareSurfaceLoader* loader, const std::string& extension);

  const SoftwareSurfaceLoader* find_loader_by_filename(const std::string& filename) const;
  const SoftwareSurfaceLoader* find_loader_by_magic(const std::string& data) const;
  const SoftwareSurfaceLoader* find_loader_by_magic(Blob const& data) const;

  SoftwareSurface from_url(const URL& url) const;
  SoftwareSurface from_file(const std::string& filename) const;
  SoftwareSurface from_file(const std::string& filename, const SoftwareSurfaceLoader* loader) const;

private:
  std::vector<std::unique_ptr<SoftwareSurfaceLoader> > m_loader;

  ExtensionMap m_extension_map;
  MimeTypeMap  m_mime_type_map;
  MagicMap m_magic_map;

private:
  SoftwareSurfaceFactory(const SoftwareSurfaceFactory&);
  SoftwareSurfaceFactory& operator=(const SoftwareSurfaceFactory&);
};

#endif

/* EOF */
