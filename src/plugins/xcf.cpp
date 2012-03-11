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

#include "plugins/xcf.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <stdio.h>

#include "math/size.hpp"
#include "plugins/png.hpp"
#include "util/exec.hpp"
#include "util/filesystem.hpp"
#include "util/log.hpp"
#include "util/url.hpp"
#include "util/raise_exception.hpp"

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

std::vector<std::string>
xcfinfo_get_layer(std::vector<char>::const_iterator start, std::vector<char>::const_iterator end)
{
  std::vector<std::string> layer_names;

  while(start != end)
  {
    std::vector<char>::const_iterator line_end = std::find(start, end, '\n');
    std::string line(&*start, line_end - start);
    start = line_end+1;
      
    char visible;
    int  width, height;
    char x_sign, y_sign;
    int  x, y;
    char color[128];
    char mode[128];
    char layer_name[1024];

    if (sscanf(line.c_str(), "%c %dx%d%c%d%c%d %127s %127s %[^\n]s",
               &visible, &width, &height, &x_sign, &x, &y_sign, &y,
               color, mode, layer_name) != 10)
    {
      raise_runtime_error("XCF::get_layer(): Couldn't parse output line:\n" + line);
    }

    layer_names.push_back(layer_name);
  }
  
  return layer_names;
}

bool
XCF::is_available()
{
  try 
  {
    std::string xcfinfo_exe = Filesystem::find_exe("xcfinfo");
    std::string xcf2png_exe = Filesystem::find_exe("xcf2png");
    log_info("found " << xcfinfo_exe << ", " << xcf2png_exe);
    return true;
  }
  catch(std::exception& err)
  {
    log_warn(err.what());
    return false;
  }  
}

std::vector<std::string>
XCF::get_layers(const URL& url)
{
  Exec xcfinfo("xcfinfo");

  if (url.has_stdio_name())
    xcfinfo.arg(url.get_stdio_name());
  else
    xcfinfo.arg("-").set_stdin(url.get_blob());

  if (xcfinfo.exec() == 0)
  {
    const std::vector<char>& stdout_lst = xcfinfo.get_stdout();
    std::vector<char>::const_iterator line_end = std::find(stdout_lst.begin(), stdout_lst.end(), '\n');
    if (line_end == stdout_lst.end())
    {
      raise_runtime_error("XCF::get_layers(): Couldn't parse output");
      return std::vector<std::string>();
    }
    else
    {
      return xcfinfo_get_layer(line_end+1, stdout_lst.end());
    }
  }
  else
  {
    raise_runtime_error("XCF::get_layers(): " + std::string(xcfinfo.get_stderr().begin(), xcfinfo.get_stderr().end()));
    return std::vector<std::string>();
  }
}

bool
XCF::get_size(const std::string& filename, Size& size)
{
  Exec xcfinfo("xcfinfo");
  xcfinfo.arg(filename);
  if (xcfinfo.exec() == 0)
  {
    const std::vector<char>& stdout_lst = xcfinfo.get_stdout();
    std::vector<char>::const_iterator line_end = std::find(stdout_lst.begin(), stdout_lst.end(), '\n');
    if (line_end == stdout_lst.end())
    {
      std::cout << "Error: XCF: couldn't parse xcfinfo output" << std::endl;
      return false;
    }
    else
    {
      std::string line(stdout_lst.begin(), line_end);
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

SoftwareSurfacePtr
XCF::load_from_file(const std::string& filename)
{
  Exec xcf2png("xcf2png");
  xcf2png.arg(filename);
  if (xcf2png.exec() != 0)
  {
    raise_runtime_error("XCF::load_from_file(): " + std::string(xcf2png.get_stderr().begin(), xcf2png.get_stderr().end()));
  }
  else
  {
    return PNG::load_from_mem(reinterpret_cast<const uint8_t*>(&*xcf2png.get_stdout().begin()),
                              xcf2png.get_stdout().size());
  }
}

SoftwareSurfacePtr
XCF::load_from_mem(void* data, int len)
{
  Exec xcf2png("xcf2png");
  xcf2png.arg("-"); // Read from stdin
  xcf2png.set_stdin(Blob::copy(data, len));
  if (xcf2png.exec() != 0)
  {
    raise_runtime_error("XCF::load_from_mem(): " + std::string(xcf2png.get_stderr().begin(), xcf2png.get_stderr().end()));
  }
  else
  {
    return PNG::load_from_mem(reinterpret_cast<const uint8_t*>(&*xcf2png.get_stdout().begin()),
                              xcf2png.get_stdout().size());
  }
}

/* EOF */
