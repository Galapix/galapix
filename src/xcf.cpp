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

#include <iostream>
#include "math/size.hpp"
#include "exec.hpp"
#include "xcf.hpp"

// Example xcfinfo output:
// Version 0, 800x800 RGB color, 6 layers, compressed RLE
//  ...
//
// Version 0, 800x800 Grayscale, 6 layers, compressed RLE
// ...
//
// Version 1, 800x800 Indexed color, 6 layers, compressed RLE
// + 800x800+0+0 Indexed-alpha Normal New Layer#3
// + 800x800+0+0 Indexed-alpha Normal New Layer
// + 800x800+0+0 Indexed-alpha Normal New Layer#2
// - 760x705+21+32 Indexed-alpha Normal Pasted Layer#1
// - 800x800+0+0 Indexed-alpha Normal Pasted Layer

bool
XCF::get_size(const std::string& filename, Size& size)
{
  Exec xcfinfo("xcfinfo");
  xcfinfo.arg(filename);
  if (xcfinfo.exec() == 0)
    {
      const std::vector<char>& stdout = xcfinfo.get_stdout();
      std::vector<char>::const_iterator line_end = std::find(stdout.begin(), stdout.end(), '\n');
      if (line_end == stdout.end())
        {
          std::cout << "Error: XCF: couldn't parse xcfinfo output" << std::endl;
          return false;
        }
      else
        {
          std::string line(stdout.begin(), line_end);
          int version, width, height;          
          if (sscanf(line.c_str(), "Version %d, %dx%d", &version, &width, &height) == 3)
            {
              size.width  = width;
              size.height = height;
              return true;
            }
          else
            {
              std::cout << "Error: XCF: couldn't parse xcfinfo output: \"" << line << "\"" << std::endl;
              return false;
            }
        }
    }
  else
    {
      std::cout.write(&*xcfinfo.get_stderr().begin(), xcfinfo.get_stderr().size());
      return false;
    }
}

SoftwareSurface
XCF::load_from_file(const std::string& filename)
{
  return SoftwareSurface();
}

/* EOF */
