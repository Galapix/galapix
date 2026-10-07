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

#ifndef HEADER_SURF_UTIL_EXEC_HPP
#define HEADER_SURF_UTIL_EXEC_HPP

#include <cstdint>
#include <span>
#include <vector>
#include <string>
#include <optional>

namespace surf {

/** The Exec class allows to call external applications in a
    conventient vasion. */
class Exec
{
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

  /** Set the working directory for executing the process */
  void set_working_directory(const std::string& path);

  /** Set what will be passed to the process on stdin

      @param blob The data passed on stdin
   */
  void set_stdin(std::vector<uint8_t> blob);

  /** Start the external program

      @return Returns the exit code of the external program
  */
  int exec();

  /** Access the stdout output of the program */
  std::span<uint8_t const> get_stdout() const { return {reinterpret_cast<uint8_t const*>(m_stdout_vec.data()), m_stdout_vec.size()}; }

  /** Access the stderr output of the program */
  std::span<uint8_t const> get_stderr() const { return {reinterpret_cast<uint8_t const*>(m_stderr_vec.data()), m_stderr_vec.size()}; }

    /** Access the stdout output of the program */
  std::string_view get_stdout_txt() const { return std::string_view(m_stdout_vec.begin(), m_stdout_vec.end()); }

  /** Access the stderr output of the program */
  std::string_view get_stderr_txt() const { return {m_stderr_vec.begin(), m_stderr_vec.end()}; }

  /** Returns a representation of the command call in simple string
      form, for visualitation only, no gurantee is made that arguments
      are properly escaped for the given shell */
  std::string str() const;

private:
  void process_io(int stdin_fd, int stdout_fd, int stderr_fd);

private:
  std::string m_program;
  bool m_absolute_path;
  std::vector<std::string> m_arguments;
  std::optional<std::string> m_working_directory;

  std::vector<char> m_stdout_vec;
  std::vector<char> m_stderr_vec;

  std::vector<uint8_t> m_stdin_data;

private:
  Exec (const Exec&);
  Exec& operator= (const Exec&);
};

} // namespace surf

#endif

/* EOF */
