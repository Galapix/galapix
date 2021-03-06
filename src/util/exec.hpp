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

#ifndef HEADER_GALAPIX_UTIL_EXEC_HPP
#define HEADER_GALAPIX_UTIL_EXEC_HPP

#include <vector>
#include <string>

#include "util/blob.hpp"

/** The Exec class allows to call external applications in a
    conventient vasion. */
class Exec
{
private:
  std::string m_program;
  bool m_absolute_path;
  std::vector<std::string> m_arguments;

  std::vector<char> m_stdout_vec;
  std::vector<char> m_stderr_vec;

  BlobPtr m_stdin_data;

public:
  static const bool ABSOLUTE_PATH = true;

  /** Construct an Exec object 

      @param program  The name or path of the program 

      @param absolute_path Set to true if \a program is an absolute
      path, if false $PATH will be searched for an application with
      the given name
  */
  Exec(const std::string& program, bool absolute_path = false);

  /** Add an argument to the list of arguments which will be used on execution 

      @param argument The argument to be passed, spaces and other
      special characters are allow, shell escaping is not needed
   */
  Exec& arg(const std::string& argument);

  /** Set what will be passed to the process on stdin 

      @param blob The data passed on stdin
   */
  void set_stdin(const BlobPtr& blob);

  /** Start the external program 
      
      @return Returns the exit code of the external program 
  */
  int exec();

  /** Access the stdout output of the program */
  const std::vector<char>& get_stdout() const { return m_stdout_vec; }

  /** Access the stderr output of the program */
  const std::vector<char>& get_stderr() const { return m_stderr_vec; }

  /** Returns a representation of the command call in simple string
      form, for visualitation only, no gurantee is made that arguments
      are properly escaped for the given shell */
  std::string str() const;

private:
  void process_io(int stdin_fd, int stdout_fd, int stderr_fd);

private:
  Exec (const Exec&);
  Exec& operator= (const Exec&);
};

#endif

/* EOF */
