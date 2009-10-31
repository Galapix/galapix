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

#include "util/imagemagick_software_surface_loader.hpp"
#include "util/jpeg_software_surface_loader.hpp"
#include "util/png_software_surface_loader.hpp"
#include "util/ufraw_software_surface_loader.hpp"
#include "util/xcf_software_surface_loader.hpp"

SoftwareSurfaceFactory::FileFormat
SoftwareSurfaceFactory::get_fileformat(const URL& url)
{
  std::string filename = url.str();

  // FIXME: Make this more clever
  if (Filesystem::has_extension(filename, ".jpg")  ||
      Filesystem::has_extension(filename, ".JPG")  ||
      Filesystem::has_extension(filename, ".jpe")  ||
      Filesystem::has_extension(filename, ".JPE")  ||
      Filesystem::has_extension(filename, ".JPEG") ||
      Filesystem::has_extension(filename, ".jpeg"))
  {
    return JPEG_FILEFORMAT;
  }
  else if (Filesystem::has_extension(filename, ".CR2"))
  {
    return UFRAW_FILEFORMAT;
  }
  else if (Filesystem::has_extension(filename, ".PNG")  ||
           Filesystem::has_extension(filename, ".png"))
  {
    return PNG_FILEFORMAT;
  }
  else if (Filesystem::has_extension(filename, ".xcf") ||
           Filesystem::has_extension(filename, ".xcf.bz2") ||
           Filesystem::has_extension(filename, ".xcf.gz"))
  {
    return XCF_FILEFORMAT;
  }
  else if (Filesystem::has_extension(filename, ".kra"))
  {
    return KRA_FILEFORMAT;
  }
  else if (Filesystem::has_extension(filename, ".svg"))
  {
    return SVG_FILEFORMAT;
  }
  else if (Filesystem::has_extension(filename, ".gif") ||
           Filesystem::has_extension(filename, ".GIF") ||
           Filesystem::has_extension(filename, ".pnm") ||
           Filesystem::has_extension(filename, ".bmp") ||
           Filesystem::has_extension(filename, ".BMP") ||
           Filesystem::has_extension(filename, ".tif") ||
           Filesystem::has_extension(filename, ".TIF") ||
           Filesystem::has_extension(filename, ".tiff") ||
           Filesystem::has_extension(filename, ".iff") ||
           Filesystem::has_extension(filename, ".IFF"))
    // FIXME: Add more stuff
  {
    return MAGICK_FILEFORMAT;
  }
  //else if (Filesystem::has_extension(filename, ".cmx")) ||
  //    {
  //    return CMX_FILEFORMAT;
  //  }
  else
  {
    return UNKNOWN_FILEFORMAT;
  }
}

SoftwareSurfacePtr
SoftwareSurfaceFactory::from_url(const URL& url)
{
  if (url.has_stdio_name())
  {
    std::string filename = url.get_stdio_name();
    std::string extension = Filesystem::get_extension(filename);

    ExtensionMap::iterator i = m_extension_map.find(extension);
    if (i == m_extension_map.end())
    {
      std::ostringstream out;
      out << "SoftwareSurfaceFactory::from_url(): " + url.str() + ": unknown file type";
      throw std::runtime_error(out.str());
    }
    else
    {
      // Try to load from file first, then fallback to loading from
      // memory blob
      SoftwareSurfacePtr surface = i->second->from_file(filename);
      if (surface)
      {
        return surface;
      }
      else
      {
        BlobPtr blob = url.get_blob();
        surface = i->second->from_mem(blob->get_data(), blob->size());
        if (surface)
        {
          return surface;
        }
        else
        {
          std::ostringstream out;
          out << "SoftwareSurfaceFactory::from_url(): " + url.str() + ": failed to load";
          throw std::runtime_error(out.str());        
        }
      }
    }
  }
  else
  {
    BlobPtr blob = url.get_blob();

    switch(get_fileformat(url))
    {
      case JPEG_FILEFORMAT:
      {
        return JPEG::load_from_mem(blob->get_data(), blob->size());
      }

      case PNG_FILEFORMAT:
      {
        return PNG::load_from_mem(blob->get_data(), blob->size());
      }

      case XCF_FILEFORMAT:
      {
        return XCF::load_from_mem(blob->get_data(), blob->size());
      }

      //           case KRA_FILEFORMAT:
      //             {
      //               BlobPtr blob = url.get_blob();
      //               return KRA::load_from_mem(blob->get_data(), blob->size());
      //             }

      case MAGICK_FILEFORMAT:
      {
        return Imagemagick::load_from_mem(blob->get_data(), blob->size());
      }
      /*
        case SVG_FILEFORMAT:
        {
        BlobPtr blob = url.get_blob();
        return RSVG::load_from_mem(blob->get_data(), blob->size());
        }            
        break;
      */

      default:
        throw std::runtime_error("SoftwareSurfaceFactory::from_url(): " + url.str() + ": unknown file type");
        return SoftwareSurfacePtr();
    }
  }
}

SoftwareSurfaceFactory::SoftwareSurfaceFactory() :
  m_loader(),
  m_extension_map(),
  m_mime_type_map()
{
  // order matters, first come, first serve, later registrations for
  // an already registered type will be ignored
  add_loader(new JPEGSoftwareSurfaceLoader);
  add_loader(new PNGSoftwareSurfaceLoader);
  add_loader(new XCFSoftwareSurfaceLoader);
  add_loader(new UFRawSoftwareSurfaceLoader);
  add_loader(new ImagemagickSoftwareSurfaceLoader);
}

SoftwareSurfaceFactory::~SoftwareSurfaceFactory()
{
}

void
SoftwareSurfaceFactory::add_loader(SoftwareSurfaceLoader* loader)
{
  m_loader.push_back(boost::shared_ptr<SoftwareSurfaceLoader>(loader));
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

/* EOF */
