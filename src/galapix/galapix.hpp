// Galapix - an image viewer for large image collections
// Copyright (C) 2008-2019 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef HEADER_GALAPIX_GALAPIX_GALAPIX_HPP
#define HEADER_GALAPIX_GALAPIX_GALAPIX_HPP

#include <vector>
#include <string>

#include "util/url.hpp"
#include "math/size.hpp"

namespace galapix {

class Options;
class System;

class Galapix final
{
public:
  Galapix(System& system);
  ~Galapix();

  void run(Options const& opts);
  int  main(int argc, char** argv);

  void cleanup(std::string const& database);
  void list(Options const& opts);
  void info(Options const& opts);
  void export_images(std::string const& database, std::vector<URL> const& urls);

private:
  System& m_system;

private:
  Galapix(Galapix const&) = delete;
  Galapix& operator=(Galapix const&) = delete;
};

} // namespace galapix

#endif

/* EOF */
