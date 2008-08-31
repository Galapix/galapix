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

#ifndef HEADER_WORKSPACE_HPP
#define HEADER_WORKSPACE_HPP

#include "image.hpp"

class Rectf;

class Workspace
{
private:
  typedef std::vector<Image> Images;
  Images images;
  Vector2i next_pos;
  int row_width;
  float progress;
  
public:
  Workspace();

  void add_image(const FileEntry& file_entry);
  void draw(const Rectf& cliprect, float scale);
  void update(float delta);

  void sort();
  void layout(float aspect_w, float aspect_h);
  void clear_cache();
  void cache_cleanup();
  void print_info();
private:
  Workspace (const Workspace&);
  Workspace& operator= (const Workspace&);
};

#endif

/* EOF */
