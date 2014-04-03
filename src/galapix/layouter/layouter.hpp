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

#ifndef HEADER_GALAPIX_GALAPIX_LAYOUTER_HPP
#define HEADER_GALAPIX_GALAPIX_LAYOUTER_HPP

#include <memory>

class ImageCollection;

class Layouter
{
private:

public:
  Layouter() {}
  virtual ~Layouter() {}

  virtual void layout(const ImageCollection& images) =0;

private:
  Layouter(const Layouter&);
  Layouter& operator=(const Layouter&);
};

typedef std::shared_ptr<Layouter> LayouterPtr;

#endif

/* EOF */
