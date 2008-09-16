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

#ifndef HEADER_EXEC_HPP
#define HEADER_EXEC_HPP

#include <vector>
#include <string>

class Exec
{
private:
  std::string program;
  std::vector<std::string> arguments;

  std::vector<char> stdout_vec;
  std::vector<char> stderr_vec;

public:
  Exec(const std::string& program);
  Exec& arg(const std::string& argument);
  int exec();

  const std::vector<char>& get_stdout() const { return stdout_vec; }
  const std::vector<char>& get_stderr() const { return stderr_vec; }

  std::string str() const;

private:
  Exec (const Exec&);
  Exec& operator= (const Exec&);
};

#endif

/* EOF */
