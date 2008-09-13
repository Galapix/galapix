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

#include <sstream>
#include <iostream>
#include <boost/bind.hpp>
#include "math/rect.hpp"
#include "tile_generator.hpp"
#include "math.hpp"
#include "jpeg.hpp"
#include "png.hpp"
#include "database_thread.hpp"
#include "tile_generator_thread.hpp"
 
TileGeneratorThread* TileGeneratorThread::current_ = 0; 

TileGeneratorThread::TileGeneratorThread()
  : Thread("TileGeneratorThread"),
    quit(false),
    min_scale(0),
    max_scale(0),
    fileid(0)
{
  current_ = this;
  working  = false;
}

TileGeneratorThread::~TileGeneratorThread()
{
}

void
TileGeneratorThread::request_tiles(const FileEntry& file_entry, int min_scale, int max_scale,
                                   const boost::function<void (TileEntry)>& callback)
{
  assert(working == false);
  working = true;

  TileGeneratorThreadJob job;

  job.entry     = file_entry;
  job.min_scale = min_scale;
  job.max_scale = max_scale;
  job.callback  = callback;

  assert(msg_queue.empty());
  msg_queue.push(job);
}

void
TileGeneratorThread::stop()
{
  quit = true;
}

void
TileGeneratorThread::process_message(const TileGeneratorThreadJob& job)
{
  if (fileid    == job.entry.get_fileid() &&
      min_scale == job.min_scale &&
      max_scale == job.max_scale)
    { 
      // FIXME: Workaround for jobs getting reqested multiple times in a row for some reason
      std::cout << "TileGeneratorThread: Job rejected: " << job.min_scale << "-" << job.max_scale << " " << job.entry.get_filename() << std::endl;
    }
  else
    {
      std::cout << "TileGeneratorThread: Processing: scale: " << job.min_scale << "-" << job.max_scale << " " << job.entry.get_filename() << "..." << std::flush;

      min_scale = job.min_scale;
      max_scale = job.max_scale;
      fileid    = job.entry.get_fileid();

      // Find scale at which the image fits on one tile
      int width  = job.entry.get_width();
      int height = job.entry.get_height();
      int scale  = job.min_scale;

      SoftwareSurface surface;

      switch(SoftwareSurface::get_fileformat(job.entry.get_filename()))
        {
          case SoftwareSurface::JPEG_FILEFORMAT:
            {
              int jpeg_scale = Math::pow2(scale);
              if (jpeg_scale > 8)
                {
                  // The JPEG class can only scale down by factor 2,4,8, so we have to
                  // limit things (FIXME: is that true? if so, why?)
                  surface = JPEG::load_from_file(job.entry.get_filename(), 8);
      
                  surface = surface.scale(Size(width  / Math::pow2(scale),
                                               height / Math::pow2(scale)));
                }
              else
                {
                  surface = JPEG::load_from_file(job.entry.get_filename(), jpeg_scale);
                }
            }
            break;

          case SoftwareSurface::PNG_FILEFORMAT:
            {
              // FIXME: This is terrible, min/max_scale are meaningless
              // for non-jpeg formats, so we should just forget them
              surface = PNG::load_from_file(job.entry.get_filename());
              surface = surface.scale(Size(width  / Math::pow2(scale),
                                           height / Math::pow2(scale)));
            }
            break;

          default:
            assert(!"Unhandled image format");
        }

      do
        {
          if (scale != job.min_scale)
            {
              surface = surface.halve();
            }

          for(int y = 0; 256*y < surface.get_height(); ++y)
            for(int x = 0; 256*x < surface.get_width(); ++x)
              {
                SoftwareSurface croped_surface = surface.crop(Rect(Vector2i(x * 256, y * 256),
                                                                   Size(256, 256)));

                job.callback(TileEntry(job.entry.get_fileid(), scale, Vector2i(x, y), croped_surface));
              }

          scale += 1;

        } while (scale <= job.max_scale);

      std::cout << "done" << std::endl;
    }
}

int
TileGeneratorThread::run()
{
  quit = false;

  TileGenerator generator;

  while(!quit)
    {
      while(!msg_queue.empty())
        {
          TileGeneratorThreadJob job = msg_queue.front();
          msg_queue.pop();
         
          try 
            {
              process_message(job);
            }
          catch(std::exception& err)
            {
              // FIXME: We need a better way to communicate errors back
              std::cout << "\nError: TileGeneratorThread: Loading failure: " << job.entry.get_filename() << std::endl;
              job.callback(TileEntry());
            }
          working = false;
        }

      //std::cout << "TileGeneratorThread: Waiting" << std::endl;
      msg_queue.wait();
    }
  
  return 0;
}

bool
TileGeneratorThread::is_working() const
{
  return working;
}

/* EOF */
