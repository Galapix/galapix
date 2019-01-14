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

#ifndef HEADER_GALAPIX_ARCHIVE_EXTRACTION_HPP
#define HEADER_GALAPIX_ARCHIVE_EXTRACTION_HPP

#include <memory>
#include <vector>

#include "util/blob.hpp"

class Extraction
{
public:
  Extraction();
  virtual ~Extraction();

  virtual std::vector<std::string> get_filenames() const = 0;
  virtual Blob get_file(const std::string& filename) const = 0;
  virtual std::string get_file_as_path(const std::string& filename) const = 0;
  virtual std::string get_type() const = 0;

private:
  Extraction(const Extraction&) = delete;
  Extraction& operator=(const Extraction&) = delete;
};

typedef std::shared_ptr<Extraction> ExtractionPtr;

#endif

/* EOF */
