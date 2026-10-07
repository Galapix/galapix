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

#ifndef HEADER_SURF_PLUGINS_FILE_JPEG_COMPRESSOR_HPP
#define HEADER_SURF_PLUGINS_FILE_JPEG_COMPRESSOR_HPP

#include <stdio.h>
#include <filesystem>
#include <string>

#include <surf/plugins/jpeg_compressor.hpp>

namespace surf {

class FileJPEGCompressor : public JPEGCompressor
{
public:
  FileJPEGCompressor(std::filesystem::path const& filename);
  ~FileJPEGCompressor() override;

private:
  FILE* m_out;

private:
  FileJPEGCompressor(const FileJPEGCompressor&);
  FileJPEGCompressor& operator=(const FileJPEGCompressor&);
};

} // namespace surf

#endif

/* EOF */
