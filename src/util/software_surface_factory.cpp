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

SoftwareSurfaceFactory* SoftwareSurfaceFactory::instance_ = 0;

SoftwareSurfaceFactory* 
SoftwareSurfaceFactory::instance()
{
  if (instance_)
  {
    return instance_;
  }
  else
  {
    instance_ = new SoftwareSurfaceFactory();
    return instance_;
  }
}

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

bool
SoftwareSurfaceFactory::get_size(const URL& url, Size& size)
{
  try 
  {
    FileFormat format = get_fileformat(url);

    if (url.has_stdio_name())
    {
      switch(format)
      {
        case JPEG_FILEFORMAT:
          size = JPEG::get_size(url.get_stdio_name());
          return true;

        case PNG_FILEFORMAT:
          return PNG::get_size(url.get_stdio_name(), size);

        case XCF_FILEFORMAT:
          return XCF::get_size(url.get_stdio_name(), size);

        case KRA_FILEFORMAT:
          return KRA::get_size(url.get_stdio_name(), size);

        case MAGICK_FILEFORMAT:
          return Imagemagick::get_size(url.get_stdio_name(), size);

        default:
          std::cout << "Warning: Using super slow get_size() for " << url.str() << std::endl;
          size = SoftwareSurfaceFactory::from_url(url)->get_size();
          return true;
      }
    }
    else
    {
      log_warning << "Warning: Using very slow SoftwareSurfaceFactory::get_size() for " << url.str() << std::endl;
      switch(format)
      {
        case JPEG_FILEFORMAT:
        {
          BlobPtr blob = url.get_blob();
          SoftwareSurfacePtr surface = JPEG::load_from_mem(blob->get_data(), blob->size());
          size = surface->get_size();
          return true;
        }

        case PNG_FILEFORMAT:
        {
          BlobPtr blob = url.get_blob();
          SoftwareSurfacePtr surface = PNG::load_from_mem(blob->get_data(), blob->size());
          size = surface->get_size();
          return true;
        }

        case XCF_FILEFORMAT:
        {
          BlobPtr blob = url.get_blob();
          SoftwareSurfacePtr surface = XCF::load_from_mem(blob->get_data(), blob->size());
          size = surface->get_size();
          return true;
        }

        case KRA_FILEFORMAT:
          //             {
          //               BlobPtr blob = url.get_blob();
          //               SoftwareSurface surface = KRA::load_from_mem(blob->get_data(), blob->size());
          //               size = surface->get_size();
          //               return false;
          //             }
          std::cout << "Krita from non file source not supported" << std::endl;
          return false;

        case MAGICK_FILEFORMAT:
        {
          BlobPtr blob = url.get_blob();
          SoftwareSurfacePtr surface = Imagemagick::load_from_mem(blob->get_data(), blob->size());
          size = surface->get_size();
          return true;
        }

        default:
          std::cout << "Warning: Using super slow get_size() for " << url.str() << std::endl;
          size = SoftwareSurfaceFactory::from_url(url)->get_size();
          return true;
      }      
    }
  }
  catch(std::exception& err)
  {
    std::cout << "Error: " << err.what() << std::endl;
    return false;
  }
}

SoftwareSurfacePtr
SoftwareSurfaceFactory::from_url(const URL& url)
{
  if (url.has_stdio_name())
  {
    switch(get_fileformat(url))
    {
      case JPEG_FILEFORMAT:
        return JPEG::load_from_file(url.get_stdio_name());

      case PNG_FILEFORMAT:
        return PNG::load_from_file(url.get_stdio_name());

      case XCF_FILEFORMAT:
        return XCF::load_from_file(url.get_stdio_name());

      case KRA_FILEFORMAT:
        return KRA::load_from_file(url.get_stdio_name());

      case MAGICK_FILEFORMAT:
        return Imagemagick::load_from_file(url.get_stdio_name());

      case SVG_FILEFORMAT:
        return RSVG::load_from_file(url.get_stdio_name());
            
      default:
        throw std::runtime_error("SoftwareSurfaceFactory::from_url(): " + url.str() + ": unknown file type");
        return SoftwareSurfacePtr();
    }  
  }
  else
  {
    switch(get_fileformat(url))
    {
      case JPEG_FILEFORMAT:
      {
        BlobPtr blob = url.get_blob();
        return JPEG::load_from_mem(blob->get_data(), blob->size());
      }

      case PNG_FILEFORMAT:
      {
        BlobPtr blob = url.get_blob();
        return PNG::load_from_mem(blob->get_data(), blob->size());
      }

      case XCF_FILEFORMAT:
      {
        BlobPtr blob = url.get_blob();
        return XCF::load_from_mem(blob->get_data(), blob->size());
      }

      //           case KRA_FILEFORMAT:
      //             {
      //               BlobPtr blob = url.get_blob();
      //               return KRA::load_from_mem(blob->get_data(), blob->size());
      //             }

      case MAGICK_FILEFORMAT:
      {
        BlobPtr blob = url.get_blob();
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
  // order matters, first come, first serve, later registratinos will
  // be ignored
  add_loader(new JPEGSoftwareSurfaceLoader);
  add_loader(new PNGSoftwareSurfaceLoader);
  add_loader(new XCFSoftwareSurfaceLoader);
  add_loader(new UFRawSoftwareSurfaceLoader);
  add_loader(new ImagemagickSoftwareSurfaceLoader);
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
SoftwareSurfaceFactory::register_by_magick(SoftwareSurfaceLoader* loader, int offset, const std::string& magick)
{
  // FIXME: implement me
}

void
SoftwareSurfaceFactory::register_by_mime_type(SoftwareSurfaceLoader* loader, const std::string& mime_type)
{
  std::map<std::string, SoftwareSurfaceLoader*>::iterator i = m_mime_type_map.find(mime_type);
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
SoftwareSurfaceFactory::register_by_extension(SoftwareSurfaceLoader* loader, const std::string& extension)
{
  std::map<std::string, SoftwareSurfaceLoader*>::iterator i = m_extension_map.find(extension);
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
