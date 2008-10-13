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

#ifndef HEADER_VIEWER_THREAD_HPP
#define HEADER_VIEWER_THREAD_HPP

#include "math/size.hpp"
#include "thread.hpp"
#include "thread_message_queue.hpp"

#include "image.hpp"
#include "job_handle.hpp"
#include "tile_entry.hpp"

class Viewer;
class FileEntry;
class Image;
class TileEntry;

class SDLViewer
{
private:
  static SDLViewer* current_;
public:
  static SDLViewer* current() { return current_; }
  
private:
  Size geometry;
  bool fullscreen;
  int  anti_aliasing;

  bool quit;

  ThreadMessageQueue<FileEntry>   file_queue;
  std::auto_ptr<Viewer> viewer;

public:
  SDLViewer(const Size& geometry, bool fullscreen, int  anti_aliasing);
  virtual ~SDLViewer();

  void run();

  void receive_file(const FileEntry& entry);

private:
  void process_event(const SDL_Event& event);

private:
  SDLViewer (const SDLViewer&);
  SDLViewer& operator= (const SDLViewer&);
};

#endif

/* EOF */
