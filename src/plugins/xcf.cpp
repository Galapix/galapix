// surf - Software surface library
// Copyright (C) 2008-2020 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or (at your
// option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
// License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include "plugins/xcf.hpp"

#include <algorithm>
#include <stdexcept>
#include <stdio.h>

#include <logmich/log.hpp>
#include <geom/size.hpp>

#include "plugins/png.hpp"
#include "software_surface_factory.hpp"
#include "software_surface_loader.hpp"
#include "util/exec.hpp"
#include "util/filesystem.hpp"

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

namespace surf {
namespace xcf {

bool is_available()
{
  try
  {
    std::string xcfinfo_exe = Filesystem::find_exe("xcfinfo");
    std::string xcf2png_exe = Filesystem::find_exe("xcf2png");
    log_info("found {}, {}", xcfinfo_exe, xcf2png_exe);
    return true;
  }
  catch(std::exception& err)
  {
    log_warn(err.what());
    return false;
  }
}

bool get_size(std::filesystem::path const& filename, geom::isize& size)
{
  Exec xcfinfo("xcfinfo");
  xcfinfo.arg(filename.string());
  if (xcfinfo.exec() == 0)
  {
    auto const& stdout_lst = xcfinfo.get_stdout();
    auto line_end = std::find(stdout_lst.begin(), stdout_lst.end(), '\n');
    if (line_end == stdout_lst.end())
    {
      std::cout << "Error: XCF: couldn't parse xcfinfo output" << std::endl;
      return false;
    }
    else
    {
      std::string line(stdout_lst.begin(), line_end);
      int version;
      int width;
      int height;
      if (sscanf(line.c_str(), "Version %d, %dx%d", &version, &width, &height) == 3)
      {
        size = geom::isize(width, height);
        return true;
      }
      else
      {
        log_error("XCF: couldn't parse xcfinfo output: \"" + line + "\"");
        return false;
      }
    }
  }
  else
  {
    std::cout << xcfinfo.get_stderr_txt();
    return false;
  }
}

SoftwareSurface load_from_file(std::filesystem::path const& filename)
{
  Exec xcf2png("xcf2png");
  xcf2png.arg(filename.string());
  if (xcf2png.exec() != 0)
  {
    throw std::runtime_error("XCF::load_from_file(): " + std::string(xcf2png.get_stderr().begin(), xcf2png.get_stderr().end()));
  }
  else
  {
    return png::load_from_mem(xcf2png.get_stdout());
  }
}

SoftwareSurface load_from_mem(std::span<uint8_t const> data)
{
  Exec xcf2png("xcf2png");
  xcf2png.arg("-"); // Read from stdin
  xcf2png.set_stdin(std::vector<uint8_t>(data.begin(), data.end()));
  if (xcf2png.exec() != 0)
  {
    throw std::runtime_error("XCF::load_from_mem(): " + std::string(xcf2png.get_stderr().begin(), xcf2png.get_stderr().end()));
  }
  else
  {
    return png::load_from_mem(xcf2png.get_stdout());
  }
}

void register_loader(SoftwareSurfaceFactory& factory)
{
  auto loader = make_loader("xcf", load_from_file, load_from_mem);

  factory.register_by_extension(*loader, "xcf");
  factory.register_by_extension(*loader, "xcf.bz2");
  factory.register_by_extension(*loader, "xcf.gz");

  factory.register_by_mime_type(*loader, "application/x-gimp-image");

  factory.register_by_magic(*loader, "gimp xcf");

  factory.add_loader(std::move(loader));
}

} // namespace xcf
} // namespace surf

/* EOF */
