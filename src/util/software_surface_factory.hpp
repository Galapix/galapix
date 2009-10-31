/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2008 Ingo Ruhnke <grumbel@gmx.de>
**
**  This program is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef HEADER_GALAPIX_UTIL_SOFTWARE_SURFACE_FACTORY_HPP
#define HEADER_GALAPIX_UTIL_SOFTWARE_SURFACE_FACTORY_HPP

#include <map>

#include "util/software_surface.hpp"

class SoftwareSurfaceLoader;
class URL;

class SoftwareSurfaceFactory
{
public:
  enum FileFormat 
  { 
    JPEG_FILEFORMAT,
    PNG_FILEFORMAT,
    XCF_FILEFORMAT, 
    KRA_FILEFORMAT, 
    SVG_FILEFORMAT, 
    MAGICK_FILEFORMAT, 
    UFRAW_FILEFORMAT, 
    UNKNOWN_FILEFORMAT 
  };

private:
  static SoftwareSurfaceFactory* instance_;

public:
  static SoftwareSurfaceFactory* instance(); 

private:
  typedef std::map<std::string, const SoftwareSurfaceLoader*> ExtensionMap;
  typedef std::map<std::string, const SoftwareSurfaceLoader*> MimeTypeMap;

  std::vector<boost::shared_ptr<SoftwareSurfaceLoader> > m_loader;
  ExtensionMap m_extension_map;
  MimeTypeMap  m_mime_type_map;
  
public:
  SoftwareSurfaceFactory();
  ~SoftwareSurfaceFactory();

  void add_loader(SoftwareSurfaceLoader* loader);
  bool has_supported_extension(const URL& url);
  
  void register_by_magick(const SoftwareSurfaceLoader* loader, int offset, const std::string& magick);
  void register_by_mime_type(const SoftwareSurfaceLoader* loader, const std::string& mime_type);
  void register_by_extension(const SoftwareSurfaceLoader* loader, const std::string& extension);

  SoftwareSurfacePtr from_url(const URL& url);

private:
  FileFormat get_fileformat(const URL& url);

private:
  SoftwareSurfaceFactory(const SoftwareSurfaceFactory&);
  SoftwareSurfaceFactory& operator=(const SoftwareSurfaceFactory&);
};

#endif

/* EOF */
