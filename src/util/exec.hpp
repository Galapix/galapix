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

#ifndef HEADER_GALAPIX_UTIL_EXEC_HPP
#define HEADER_GALAPIX_UTIL_EXEC_HPP

#include <vector>
#include <string>
#include <optional>

#include "util/blob.hpp"

namespace galapix {

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
  Exec(std::string const& program, bool absolute_path = false);

  /** Add an argument to the list of arguments which will be used on execution

      @param argument The argument to be passed, spaces and other
      special characters are allow, shell escaping is not needed
   */
  Exec& arg(std::string const& argument);

  /** Set the working directory for executing the process */
  void set_working_directory(std::string const& path);

  /** Set what will be passed to the process on stdin

      @param blob The data passed on stdin
   */
  void set_stdin(Blob const& blob);

  /** Start the external program

      @return Returns the exit code of the external program
  */
  int exec();

  /** Access the stdout output of the program */
  std::span<uint8_t const> get_stdout() const { return {m_stdout_vec.data(), m_stdout_vec.size()}; }

  /** Access the stderr output of the program */
  std::span<uint8_t const> get_stderr() const { return {m_stderr_vec.data(), m_stderr_vec.size()}; }

  std::vector<uint8_t>&& move_stdout() { return std::move(m_stdout_vec); }
  std::vector<uint8_t>&& move_stderr() { return std::move(m_stderr_vec); }

    /** Access the stdout output of the program */
  std::string_view get_stdout_txt() const { return {reinterpret_cast<char const*>(m_stdout_vec.data()), m_stdout_vec.size()}; }

  /** Access the stderr output of the program */
  std::string_view get_stderr_txt() const { return {reinterpret_cast<char const*>(m_stderr_vec.data()), m_stderr_vec.size()}; }

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

  std::vector<uint8_t> m_stdout_vec;
  std::vector<uint8_t> m_stderr_vec;

  Blob m_stdin_data;

private:
  Exec (Exec const&);
  Exec& operator= (Exec const&);
};

} // namespace galapix

#endif

/* EOF */
