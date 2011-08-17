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

#include "util/software_surface_factory.hpp"

#include <sstream>
#include <stdexcept>

#include "util/filesystem.hpp"
#include "util/log.hpp"
#include "util/software_surface_loader.hpp"
#include "util/url.hpp"

#include "plugins/imagemagick.hpp"
#include "plugins/jpeg.hpp"
#include "plugins/kra.hpp"
#include "plugins/png.hpp"
#include "plugins/rsvg.hpp"
#include "plugins/xcf.hpp"

#include "util/dds_software_surface_loader.hpp"
#include "util/imagemagick_software_surface_loader.hpp"
#include "util/jpeg_software_surface_loader.hpp"
#include "util/kra_software_surface_loader.hpp"
#include "util/png_software_surface_loader.hpp"
#include "util/rsvg_software_surface_loader.hpp"
#include "util/ufraw_software_surface_loader.hpp"
#include "util/xcf_software_surface_loader.hpp"

SoftwareSurfaceFactory::SoftwareSurfaceFactory() :
  m_loader(),
  m_extension_map(),
  m_mime_type_map()
{
  // order matters, first come, first serve, later registrations for
  // an already registered type will be ignored
  add_loader(new JPEGSoftwareSurfaceLoader);
  add_loader(new PNGSoftwareSurfaceLoader);

  if (XCF::is_available())
    add_loader(new XCFSoftwareSurfaceLoader);

  if (UFRaw::is_available())
  add_loader(new UFRawSoftwareSurfaceLoader);

  if (RSVG::is_available())
    add_loader(new RSVGSoftwareSurfaceLoader);

  if (KRA::is_available())
    add_loader(new KRASoftwareSurfaceLoader);

  add_loader(new DDSSoftwareSurfaceLoader);

  add_loader(new ImagemagickSoftwareSurfaceLoader);
}

SoftwareSurfaceFactory::~SoftwareSurfaceFactory()
{
}

void
SoftwareSurfaceFactory::add_loader(SoftwareSurfaceLoader* loader)
{
  m_loader.push_back(std::shared_ptr<SoftwareSurfaceLoader>(loader));
  loader->register_loader(*this);
}

bool
SoftwareSurfaceFactory::has_supported_extension(const URL& url)
{
  std::string extension = Filesystem::get_extension(url.str());
  ExtensionMap::iterator i = m_extension_map.find(extension);
  return (i != m_extension_map.end());
}

void
SoftwareSurfaceFactory::register_by_magick(const SoftwareSurfaceLoader* loader, int offset, const std::string& magick)
{
  // FIXME: implement me
}

void
SoftwareSurfaceFactory::register_by_mime_type(const SoftwareSurfaceLoader* loader, const std::string& mime_type)
{
  MimeTypeMap::iterator i = m_mime_type_map.find(mime_type);
  if (i == m_mime_type_map.end())
  {
    m_mime_type_map[mime_type] = loader;
  }
  else
  {
    // ignoring registration if something is already registered
  }
}

void
SoftwareSurfaceFactory::register_by_extension(const SoftwareSurfaceLoader* loader, const std::string& extension)
{
  ExtensionMap::iterator i = m_extension_map.find(extension);
  if (i == m_extension_map.end())
  {
    m_extension_map[extension] = loader;
  }
  else
  {
    // ignoring registration if something is already registered
  }
}

SoftwareSurfacePtr
SoftwareSurfaceFactory::from_file(const std::string& filename) const
{
  std::string extension = Filesystem::get_extension(filename);

  ExtensionMap::const_iterator i = m_extension_map.find(extension);
  if (i == m_extension_map.end())
  {
    std::ostringstream out;
    out << "SoftwareSurfaceFactory::from_file(): " << filename << ": unknown file type";
    throw std::runtime_error(out.str());
  }
  else
  {
    const SoftwareSurfaceLoader& loader = *i->second;

    if (loader.supports_from_file())
    {
      return loader.from_file(filename);
    }
    else if (loader.supports_from_mem())
    {
      BlobPtr blob = Blob::from_file(filename);
      return loader.from_mem(blob->get_data(), blob->size());
    }
    else
    {
      throw std::runtime_error("SoftwareSurfaceFactory::from_file(): loader does not support loading");
    }
  }
}

SoftwareSurfacePtr
SoftwareSurfaceFactory::from_url(const URL& url) const
{
  log_debug << url << std::endl;

  if (url.has_stdio_name())
  {
    return from_file(url.get_stdio_name());
  }
  else
  {
    std::string mime_type;
    BlobPtr blob = url.get_blob(&mime_type);

    const SoftwareSurfaceLoader* loader = 0;

    // try to find a loader by mime-type
    if (!mime_type.empty())
    {
      MimeTypeMap::const_iterator i = m_mime_type_map.find(mime_type);
      if (i != m_mime_type_map.end())
      {
        loader = i->second;
      }
    }

    // try to find a loader by file extension
    if (!loader)
    {
      std::string extension = Filesystem::get_extension(url.str());
      ExtensionMap::const_iterator i = m_extension_map.find(extension);
      if (i != m_extension_map.end())
      {
        loader = i->second;
      }
    }

    // load the image or fail if no loader is present
    if (!loader)
    {
      std::ostringstream out;
      out << "SoftwareSurfaceFactory::from_url(): " << url.str() << ": unknown file type";
      throw std::runtime_error(out.str());      
    }
    else
    {
      if (loader->supports_from_mem())
      {
        return loader->from_mem(blob->get_data(), blob->size()); 
      }
      else
      {
        std::ostringstream out;
        out << "SoftwareSurfaceFactory::from_url(): " << url.str() << ": loader doesn't support from_mem(), workaround not implemented";
        throw std::runtime_error(out.str());        
      }
    }
  }
}

/* EOF */
