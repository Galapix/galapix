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

#include "util/exec.hpp"

#include <string.h>
#include <errno.h>
#include <stdexcept>
#include <sys/wait.h>
#include <sstream>

Exec::Exec(const std::string& program, bool absolute_path) :
  m_program(program),
  m_absolute_path(absolute_path),
  m_arguments(),
  m_stdout_vec(),
  m_stderr_vec(),
  m_stdin_data()
{
}

Exec&
Exec::arg(const std::string& argument)
{
  m_arguments.push_back(argument);
  return *this;
}

void
Exec::set_stdin(const BlobPtr& blob)
{
  m_stdin_data = blob;
}

int
Exec::exec()
{
  int stdin_fd[2];
  int stdout_fd[2];
  int stderr_fd[2];

  if (pipe(stdout_fd) < 0 || pipe(stderr_fd) < 0 || pipe(stdin_fd) < 0)
    throw std::runtime_error("Exec:exec(): pipe failed");

  pid_t pid = fork();
  if (pid < 0)
  { // error
    // Cleanup
    close(stdout_fd[0]);
    close(stdout_fd[1]);
    close(stderr_fd[0]);
    close(stderr_fd[1]);
    close(stdin_fd[0]);
    close(stdin_fd[1]);

    throw std::runtime_error("Exec::exec(): fork failed");
  }
  else if (pid == 0) 
  { // child
    close(stdin_fd[1]);
    close(stdout_fd[0]);
    close(stderr_fd[0]);

    dup2(stdin_fd[0], STDIN_FILENO);
    close(stdin_fd[0]);

    dup2(stdout_fd[1], STDOUT_FILENO);
    close(stdout_fd[1]);

    dup2(stderr_fd[1], STDERR_FILENO);
    close(stderr_fd[1]);

    // Create C-style array for arguments 
    boost::scoped_array<char*> c_arguments(new char*[m_arguments.size()+2]);
    c_arguments[0] = strdup(m_program.c_str());
    for(std::vector<std::string>::size_type i = 0; i < m_arguments.size(); ++i)
      c_arguments[i+1] = strdup(m_arguments[i].c_str());
    c_arguments[m_arguments.size()+1] = NULL;
      
    // Execute the program
    if (m_absolute_path)
    {
      execv(c_arguments[0], c_arguments.get());
    }
    else
    {
      execvp(c_arguments[0], c_arguments.get());
    }
      
    int error_code = errno;

    // Cleanup
    for(int i = 0; c_arguments[i] != NULL; ++i)
      free(c_arguments[i]);

    // execvp() only returns on failure 
    throw std::runtime_error("Exec::exec(): " + m_program + ": " + strerror(error_code));
  }
  else // if (pid > 0)
  { // parent
    close(stdin_fd[0]);
    close(stdout_fd[1]);
    close(stderr_fd[1]);

    int len;
    char buffer[4096];
      
    if (m_stdin_data)
    {
      if (write(stdin_fd[1], m_stdin_data->get_data(), m_stdin_data->size()) < 0)
      {
        throw std::runtime_error(strerror(errno));
      }
    }

    close(stdin_fd[1]);

    while((len = read(stdout_fd[0], buffer, sizeof(buffer))) > 0)
    {
      m_stdout_vec.insert(m_stdout_vec.end(), buffer, buffer+len);
    }

    if (len == -1)
    {
      std::ostringstream out;
      out << "Exec::exec(): error reading stdout from: " << str();
      throw std::runtime_error(out.str());
    }

    while((len = read(stderr_fd[0], buffer, sizeof(buffer))) > 0)
    {
      m_stderr_vec.insert(m_stderr_vec.end(), buffer, buffer+len);
    }

    if (len == -1)
    {
      std::ostringstream out;
      out << "Exec::exec(): error reading stderr from: " << str();
      throw std::runtime_error(out.str());
    }

    int child_status = 0;
    waitpid(pid, &child_status, 0);

    // Cleanup
    close(stdout_fd[0]);
    close(stderr_fd[0]);

    return WEXITSTATUS(child_status);
  }
}

std::string
Exec::str() const
{
  std::ostringstream out;

  out << m_program << " ";

  for(std::vector<std::string>::size_type i = 0; i < m_arguments.size(); ++i)
    out << "'" << m_arguments[i] << "' ";

  return out.str();
}

/* EOF */
