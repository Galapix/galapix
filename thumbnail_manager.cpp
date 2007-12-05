/*  $Id$
**   __      __ __             ___        __   __ __   __
**  /  \    /  \__| ____    __| _/_______/  |_|__|  | |  |   ____
**  \   \/\/   /  |/    \  / __ |/  ___/\   __\  |  | |  | _/ __ \
**   \        /|  |   |  \/ /_/ |\___ \  |  | |  |  |_|  |_\  ___/
**    \__/\  / |__|___|  /\____ /____  > |__| |__|____/____/\___  >
**         \/          \/      \/    \/                         \/
**  Copyright (C) 2007 Ingo Ruhnke <grumbel@gmx.de>
**
**  This program is free software; you can redistribute it and/or
**  modify it under the terms of the GNU General Public License
**  as published by the Free Software Foundation; either version 2
**  of the License, or (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
** 
**  You should have received a copy of the GNU General Public License
**  along with this program; if not, write to the Free Software
**  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
**  02111-1307, USA.
*/

#include "epeg/Epeg.h"
#include <sstream>
#include <iostream>
#include <stdexcept>
#include "SDL_image.h"
#include "filesystem.hpp"
#include "md5.hpp"
#include "software_surface.hpp"
#include "thumbnail_manager.hpp"

ThumbnailManager::ThumbnailManager()
{
  
}

ThumbnailManager::~ThumbnailManager()
{
  std::cout << "destroying ThumbnailManager" << std::endl;
}

SoftwareSurface* 
ThumbnailManager::get_by_url(const std::string& url, int thumb_size) // URL is file://... *not* just a pathname
{
  if (thumb_size == -1) // load original
    {
      try 
        { 
          return new SoftwareSurface(url.substr(7)); // cut file:// part
        }
      catch(std::exception& err) 
        {
          std::cout << "Loader: " << err.what() << std::endl;
          return 0;
        }
    }
  else
    {
      std::string md5 = MD5::md5_string(url);

      std::ostringstream out;
      out << Filesystem::get_home() << "/.griv/cache/by_url/"
          << thumb_size << "/" << md5.substr(0,2) << "/" << md5.substr(2) << ".jpg";
  
      std::string thumb_filename = out.str();
  
      try 
        {
          return new SoftwareSurface(thumb_filename);
        }
      catch(std::exception& err) 
        {
          // Try to generate the thumbnail
          try {
            generate(url.substr(7), thumb_filename, thumb_size); // cut file:// part
          } catch (std::exception& err) {
            std::cout << err.what() << std::endl;
            return 0;
          }

          // FIXME: This will fail for some images, which ones?
          try {
            return new SoftwareSurface(thumb_filename.c_str());
          } catch(std::exception& err) {
            std::cout << "ThumbnailManager: FIXME: This shouldn't happen: " << err.what() << "\n"
                      << "  - for " << url
                      << std::endl;
            return 0;
          }
        }
    }
}

void
ThumbnailManager::generate(const std::string& filename, const std::string& thumb_location, int thumb_size)
{
  Epeg_Image* img = epeg_file_open(filename.c_str());
  if (!img)
    {
      throw std::runtime_error("ThumbnailManager::generate: Cannot open " + filename);
    }
  
  int w, h;
  epeg_size_get(img, &w, &h); // FIXME: Should use FileEntryCache instead

  if (w > thumb_size || h > thumb_size) 
    {
      epeg_quality_set(img, 80);
      epeg_thumbnail_comments_enable(img, 1);

      if (w > h)
        epeg_decode_size_set(img, thumb_size, thumb_size * h / w);
      else
        epeg_decode_size_set(img, thumb_size * w / h, thumb_size);
  
      epeg_decode_colorspace_set(img, EPEG_RGB8);

      std::cout << "." << std::flush;
      std::cout << thumb_size << " - " << filename << " => " << thumb_location << std::endl;

      epeg_file_output_set(img, thumb_location.c_str());
      epeg_encode(img);
    }
  else
    {
      
    }
      
  epeg_close(img);
}

/* EOF */
