/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2012 Ingo Ruhnke <grumbel@gmail.com>
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

#ifndef HEADER_GALAPIX_RESOURCE_VIDEO_INFO_HPP
#define HEADER_GALAPIX_RESOURCE_VIDEO_INFO_HPP

class VideoInfo
{
private:
  int m_width;
  int m_height;
  float m_duration;
  float m_aspect_ratio;

public:
  VideoInfo() :
    m_width(),
    m_height(),
    m_duration(),
    m_aspect_ratio()
  {}

  int get_width() const { return m_width; }
  int get_height() const { return m_height; }
  float get_duration() const { return m_duration; }
  float get_aspect_ratio() const { return m_aspect_ratio; }
};

#endif

/* EOF */
