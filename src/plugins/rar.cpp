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

#include <stdexcept>
#include "../exec.hpp"
#include "../blob.hpp"
#include "rar.hpp"

std::vector<std::string>
Rar::get_filenames(const std::string& rar_filename)
{
  Exec rar("rar");
  rar.arg("vb").arg("-p-").arg(rar_filename);
  if (rar.exec() == 0)
    {
      std::vector<std::string> lst;
      const std::vector<char>& stdout = rar.get_stdout();
      std::vector<char>::const_iterator start = stdout.begin();
      for(std::vector<char>::const_iterator i = stdout.begin(); i != stdout.end(); ++i)
        {
          if (*i == '\n')
            {
              lst.push_back(std::string(start, i));
              start = i+1;
            }
        }
      return lst;
    }
  else
    {
      throw std::runtime_error("Rar: " + std::string(rar.get_stderr().begin(), rar.get_stderr().end()));
      return std::vector<std::string>();
    }
}

Blob
Rar::get_file(const std::string& rar_filename, const std::string& filename)
{
  Exec rar("rar");
  rar.arg("p").arg("-inul").arg("-p-").arg(rar_filename).arg(filename);
  if (rar.exec() == 0)
    {
      // FIXME: Unneeded copy of data
      return Blob::copy(&*rar.get_stdout().begin(), rar.get_stdout().size());
    }
  else
    {
      throw std::runtime_error("Rar: " + rar.str() + "\n" + std::string(rar.get_stderr().begin(), rar.get_stderr().end()));
      return Blob();
    }
}

#ifdef __TEST_RAR__

// g++ -Wall -Werror -ansi -pedantic blob.cpp exec.cpp rar.cpp -o myrar -D__TEST_RAR__  

#include <iostream>

int main(int argc, char** argv)
{
  try
    {
      if (argc == 2)
        {
          const std::vector<std::string>& files = Rar::get_filenames(argv[1]);
          for(std::vector<std::string>::const_iterator i = files.begin(); i != files.end(); ++i)
            {
              std::cout << "File: '" << *i << "'" << std::endl;
            }
        }
      else if (argc == 3)
        {
          Blob blob = Rar::get_file(argv[1], argv[2]);
          blob.write_to_file("/tmp/out.file");
          std::cout << "Writting /tmp/out.file" << std::endl;
        }
      else 
        {
          std::cout << "Usage: " << argv[0] << " RARFILE" << std::endl;
          std::cout << "       " << argv[0] << " RARFILE FILETOEXTRACT" << std::endl;
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
