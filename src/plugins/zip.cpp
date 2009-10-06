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

#include "plugins/zip.hpp"

#include <stdexcept>

#include "util/exec.hpp"

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

std::vector<std::string>
Zip::get_filenames(const std::string& zip_filename)
{
  Exec unzip("unzip");
  unzip.arg("-lqq").arg(zip_filename);
  if (unzip.exec() == 0)
    {
      std::vector<std::string> lst;
      unzip_parse_output(unzip.get_stdout().begin(), unzip.get_stdout().end(), lst);
      return lst;
    }
  else
    {
      throw std::runtime_error("Zip: " + std::string(unzip.get_stderr().begin(), unzip.get_stderr().end()));
      return std::vector<std::string>();
    }
}

Blob
Zip::get_file(const std::string& zip_filename, const std::string& filename)
{
  Exec unzip("unzip");
  unzip.arg("-pqq").arg(zip_filename).arg(filename);
  if (unzip.exec() == 0)
    {
      // FIXME: Unneeded copy of data
      return Blob::copy(&*unzip.get_stdout().begin(), unzip.get_stdout().size());
    }
  else
    {
      throw std::runtime_error("Zip: " + std::string(unzip.get_stderr().begin(), unzip.get_stderr().end()));
      return Blob();
    }
}

#ifdef __TEST_ZIP__

// g++ -Wall -Werror -ansi -pedantic blob.cpp exec.cpp zip.cpp -o myzip -D__TEST_ZIP__  

#include <iostream>

int main(int argc, char** argv)
{
  try
    {
      if (argc == 2)
        {
          const std::vector<std::string>& files = Zip::get_filenames(argv[1]);
          for(std::vector<std::string>::const_iterator i = files.begin(); i != files.end(); ++i)
            {
              std::cout << "File: '" << *i << "'" << std::endl;
            }
        }
      else if (argc == 3)
        {
          Blob blob = Zip::get_file(argv[1], argv[2]);
          blob.write_to_file("/tmp/out.file");
          std::cout << "Writting /tmp/out.file" << std::endl;
        }
      else 
        {
          std::cout << "Usage: " << argv[0] << " ZIPFILE" << std::endl;
          std::cout << "       " << argv[0] << " ZIPFILE FILETOEXTRACT" << std::endl;
        }
    }
  catch(std::exception& err) 
    {
      std::cout << "Error: " << err.what() << std::endl;
    }
  return 0;
}

#endif

/* EOF */
