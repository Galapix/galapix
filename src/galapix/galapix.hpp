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

class Options;
class System;

class Galapix final
{
public:
  Galapix(System& system);
  ~Galapix();

  void run(const Options& opts);
  int  main(int argc, char** argv);

  void cleanup(const std::string& database);
  void list(const Options& opts);
  void info(const Options& opts);
  void export_images(const std::string& database, const std::vector<URL>& urls);

private:
  System& m_system;

private:
  Galapix(const Galapix&) = delete;
  Galapix& operator=(const Galapix&) = delete;
};

#endif

/* EOF */
