/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2008 Ingo Ruhnke <grumbel@gmail.com>
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
#include <logmich/log.hpp>

#include "util/filesystem.hpp"
#include "util/raise_exception.hpp"
#include "util/software_surface_loader.hpp"
#include "util/url.hpp"

#include "plugins/imagemagick.hpp"
#include "plugins/jpeg.hpp"
#include "plugins/kra.hpp"
#include "plugins/png.hpp"
#include "plugins/rsvg.hpp"
#include "plugins/vidthumb.hpp"
#include "plugins/xcf.hpp"

#include "util/dds_software_surface_loader.hpp"
#include "util/imagemagick_software_surface_loader.hpp"
#include "util/jpeg_software_surface_loader.hpp"
#include "util/kra_software_surface_loader.hpp"
#include "util/png_software_surface_loader.hpp"
#include "util/rsvg_software_surface_loader.hpp"
#include "util/ufraw_software_surface_loader.hpp"
#include "util/vidthumb_software_surface_loader.hpp"
#include "util/xcf_software_surface_loader.hpp"

namespace {
bool has_prefix(const std::string& lhs, const std::string& rhs)
{
  if (lhs.length() < rhs.length())
    return false;
  else
    return lhs.compare(0, rhs.length(), rhs) == 0;
}
} // namespace

SoftwareSurfaceFactory::SoftwareSurfaceFactory() :
  m_loader(),
  m_extension_map(),
  m_mime_type_map(),
  m_magic_map()
{
  // order matters, first come, first serve, later registrations for
  // an already registered type will be ignored
  add_loader(std::make_unique<JPEGSoftwareSurfaceLoader>());
  add_loader(std::make_unique<PNGSoftwareSurfaceLoader>());

  if (XCF::is_available())
    add_loader(std::make_unique<XCFSoftwareSurfaceLoader>());

  if (UFRaw::is_available())
    add_loader(std::make_unique<UFRawSoftwareSurfaceLoader>());

  if (RSVG::is_available())
    add_loader(std::make_unique<RSVGSoftwareSurfaceLoader>());

  if (VidThumb::is_available())
    add_loader(std::make_unique<VidThumbSoftwareSurfaceLoader>());

  if (KRA::is_available())
    add_loader(std::make_unique<KRASoftwareSurfaceLoader>());

  add_loader(std::make_unique<DDSSoftwareSurfaceLoader>());

  add_loader(std::make_unique<ImagemagickSoftwareSurfaceLoader>());
}

SoftwareSurfaceFactory::~SoftwareSurfaceFactory()
{
}

void
SoftwareSurfaceFactory::add_loader(std::unique_ptr<SoftwareSurfaceLoader> loader)
{
  m_loader.push_back(std::move(loader));
  m_loader.back()->register_loader(*this);
}

bool
SoftwareSurfaceFactory::has_supported_extension(const URL& url)
{
  std::string extension = Filesystem::get_extension(url.str());
  ExtensionMap::iterator i = m_extension_map.find(extension);
  return (i != m_extension_map.end());
}

void
SoftwareSurfaceFactory::register_by_magic(const SoftwareSurfaceLoader* loader, const std::string& magic)
{
  auto it = m_magic_map.find(magic);
  if (it == m_magic_map.end())
  {
    m_magic_map[magic] = loader;
  }
  else
  {
    // ignoring registration if something is already registered
  }
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

const SoftwareSurfaceLoader*
SoftwareSurfaceFactory::find_loader_by_filename(const std::string& filename) const
{
  std::string extension = Filesystem::get_extension(filename);
  const auto& it = m_extension_map.find(extension);
  if (it == m_extension_map.end())
  {
    return nullptr;
  }
  else
  {
    return it->second;
  }
}

const SoftwareSurfaceLoader*
SoftwareSurfaceFactory::find_loader_by_magic(const std::string& data) const
{
  for(const auto& it: m_magic_map)
  {
    if (has_prefix(data, it.first))
    {
      return it.second;
    }
  }
  return nullptr;
}

const SoftwareSurfaceLoader*
SoftwareSurfaceFactory::find_loader_by_magic(const BlobPtr& data) const
{
  size_t size = std::min(static_cast<size_t>(1024), data->size());
  return find_loader_by_magic(std::string(reinterpret_cast<const char*>(data->get_data()), size));
}

SoftwareSurfacePtr
SoftwareSurfaceFactory::from_file(const std::string& filename, const SoftwareSurfaceLoader* loader) const
{
  assert(loader);

  if (loader->supports_from_file())
  {
    return loader->from_file(filename);
  }
  else if (loader->supports_from_mem())
  {
    BlobPtr blob = Blob::from_file(filename);
    return loader->from_mem(blob->get_data(), blob->size());
  }
  else
  {
    raise_exception(std::runtime_error, "'" << loader->get_name() << "' loader does not support loading");
  }
}

SoftwareSurfacePtr
SoftwareSurfaceFactory::from_file(const std::string& filename) const
{
  const SoftwareSurfaceLoader* loader = find_loader_by_filename(filename);
  if (!loader)
  {
    std::ostringstream out;
    out << "SoftwareSurfaceFactory::from_file(): " << filename << ": unknown file type";
    raise_runtime_error(out.str());
  }
  else
  {
    try
    {
      return from_file(filename, loader);
    }
    catch (const std::exception& err)
    {
      // retry with magic
      std::string magic = Filesystem::get_magic(filename);
      auto new_loader = find_loader_by_magic(magic);
      if (new_loader && new_loader != loader)
      {
        log_warn("%1%: file extension error, file is a %2%", filename, new_loader->get_name());
        return from_file(filename, new_loader);
      }
      else
      {
        throw;
      }
    }
  }
}

SoftwareSurfacePtr
SoftwareSurfaceFactory::from_url(const URL& url) const
{
  log_debug("%1%", url);

  if (url.has_stdio_name())
  {
    return from_file(url.get_stdio_name());
  }
  else
  {
    std::string mime_type;
    BlobPtr blob = url.get_blob(&mime_type);

    const SoftwareSurfaceLoader* loader = nullptr;

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
      raise_runtime_error(out.str());
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
        raise_runtime_error(out.str());
      }
    }
  }
}

/* EOF */
