/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2008 Ingo Ruhnke <grumbel@gmail.com>
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

#include "archive/zip.hpp"

#include <stdexcept>
#include <sstream>

#include "util/exec.hpp"
#include "util/raise_exception.hpp"

namespace {

std::string zip_error_to_string(int err)
{
  // codes are taken from the unzip manpage
  switch(err)
  {
    case 0:
      return "normal; no errors or warnings detected.";

    case 1:
      return "out one or more warning errors were encountered, but processing "
        "completed successfully anyway. This includes zipfiles where one or "
        "more files was skipped due to unsupported compression method or "
        "encryption with an unknown password.";

    case 2:
      return "a generic error in the zipfile format was detected.  Processing may "
        "have completed successfully anyway; some broken zipfiles created by "
        "other archivers have simple work-arounds.";

    case 3:
      return "a severe error in the zipfile format was detected.  Processing "
        "probably failed immediately.";

    case 4:
      return "unzip was unable to allocate memory for one or more buffers during "
        "program initialization.";

    case 5:
      return "unzip was unable to allocate memory or unable to obtain a tty to read "
        "the decryption password(s).";

    case 6:
      return "unzip was unable to allocate memory during decompression to disk.";

    case 7:
      return "unzip was unable to allocate memory during in-memory decompression.";

    case 8:
      return "[currently not used]";

    case 9:
      return "the specified zipfiles were not found.";

    case 10:
      return "invalid options were specified on the command line.";

    case 11:
      return "no matching files were found.";

    case 50:
      return "the disk is (or was) full during extraction.";

    case 51:
      return "the end of the ZIP archive was encountered prematurely.";
      break;

    case 80:
      return "the user aborted unzip prematurely with control-C (or similar)";

    case 81:
      return "testing or extraction of one or more files failed due to unsupported "
        "compression methods or unsupported decryption.";

    case 82:
      return "no files were found due to bad decryption password(s).  (If even one "
        "file is successfully processed, however, the exit status is 1.)";

    default:
    {
      std::ostringstream out;
      out << "unknown error code: "<< err;
      return out.str();
    }
  }
}

void unzip_parse_line(std::vector<char>::const_iterator start, std::vector<char>::const_iterator end,
                      std::vector<std::string>& lst)
{

  if (start != end && *(end-1) == '/')
  { // Do nothing if the given entry is a directory
    return;
  }
  else
  { // Figure out where the filename starts
    bool in_whitespace = true;
    int  column = 0;
    for(std::vector<char>::const_iterator i = start; i != end; ++i)
    {
      if (in_whitespace)
      {
        if (*i != ' ')
        {
          column += 1;
          in_whitespace = false;
          if (column == 4)
          {
            lst.push_back(std::string(i, end));
          }
        }
      }
      else
      {
        if (*i == ' ')
        {
          in_whitespace = true;
        }
      }
    }
  }
}

void unzip_parse_output(std::vector<char>::const_iterator start, std::vector<char>::const_iterator end,
                        std::vector<std::string>& lst)
{
  std::vector<char>::const_iterator line_start = start;
  for(std::vector<char>::const_iterator i = start; i != end; ++i)
  {
    if (*i == '\n')
    {
      unzip_parse_line(line_start, i, lst);
      line_start = i + 1;
    }
  }
}

} // namespace

std::vector<std::string>
Zip::get_filenames(const std::string& archive)
{
  Exec unzip("unzip");
  unzip.arg("-lqq").arg(archive);
  int zip_return_code = unzip.exec();
  if (zip_return_code == 0)
  {
    std::vector<std::string> lst;
    unzip_parse_output(unzip.get_stdout().begin(), unzip.get_stdout().end(), lst);
    return lst;
  }
  else
  {
    std::ostringstream out;
    out << "Zip::get_filenames(): " << unzip.str()
        << "\n  zip-exit-code: " << zip_error_to_string(zip_return_code)
        << "\n  " << std::string(unzip.get_stderr().begin(), unzip.get_stderr().end());
    raise_runtime_error(out.str());
  }
}

Blob
Zip::get_file(const std::string& archive, const std::string& filename_in)
{
  // unzip uses wildcard expressions, not raw filenames, thus we have
  // to escape a few special characters
  std::string filename;
  for(std::string::const_iterator i = filename_in.begin(); i != filename_in.end(); ++i)
  {
    if (*i == '[' ||
        *i == ']' ||
        *i == '?' ||
        *i == '*')
    {
      filename += '\\';
      filename += *i;
    }
    else
    {
      filename += *i;
    }
  }

  Exec unzip("unzip");
  unzip.arg("-pqq").arg(archive).arg(filename);
  int zip_return_code = unzip.exec();
  if (zip_return_code == 0)
  {
    // FIXME: Unneeded copy of data
    return Blob::copy(&*unzip.get_stdout().begin(), unzip.get_stdout().size());
  }
  else
  {
    // "unzip -p" does not report any error messages at all, so we
    // have to do them manually
    std::ostringstream out;
    out << "Zip::get_file(): " << unzip.str()
        << "\n  zip-exit-code: " << zip_error_to_string(zip_return_code)
        << "\n  " << std::string(unzip.get_stderr().begin(), unzip.get_stderr().end());
    raise_runtime_error(out.str());
  }
}

void
Zip::extract(const std::string& archive, const std::string& target_directory)
{
  Exec unzip("unzip");
  unzip.arg("-qq").arg(archive).arg("-d").arg(target_directory).arg(archive);
  int zip_return_code = unzip.exec();
  if (zip_return_code != 0)
  {
    // "unzip -p" does not report any error messages at all, so we
    // have to do them manually
    std::ostringstream out;
    out << "Zip::extract(): " << unzip.str()
        << "\n  zip-exit-code: " << zip_error_to_string(zip_return_code)
        << "\n  " << std::string(unzip.get_stderr().begin(), unzip.get_stderr().end());
    raise_runtime_error(out.str());
  }
}

/* EOF */
