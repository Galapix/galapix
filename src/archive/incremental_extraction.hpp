/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2012 Ingo Ruhnke <grumbel@gmx.de>
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

#ifndef HEADER_GALAPIX_ARCHIVE_INCREMENTAL_EXTRACTION_HPP
#define HEADER_GALAPIX_ARCHIVE_INCREMENTAL_EXTRACTION_HPP

#include <boost/filesystem.hpp>

#include "archive/extraction.hpp"

template<typename Extractor>
class IncrementalExtraction : public Extraction
{
private:
  std::string m_archive;

public:
  IncrementalExtraction(const std::string& archive) :
    m_archive(archive)
  {}

  std::vector<std::string> get_filenames() const
  {
    return Extractor::get_filenames(m_archive);
  }

  BlobPtr get_file(const std::string& filename) const
  {
    return Extractor::get_file(m_archive, filename);
  }

  std::string get_file_as_path(const std::string& filename) const
  {
    BlobPtr blob = Extractor::get_file(m_archive, filename);
    boost::filesystem::path path = boost::filesystem::temp_directory_path() /
      boost::filesystem::unique_path("%%%%-%%%%-%%%%-%%%%");
    blob->write_to_file(path.string());
    return path.string();
  }

private:
  IncrementalExtraction(const IncrementalExtraction&);
  IncrementalExtraction& operator=(const IncrementalExtraction&);
};

#endif

/* EOF */
