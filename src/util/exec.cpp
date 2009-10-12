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

const bool Exec::ABSOLUTE_PATH = true;

Exec::Exec(const std::string& program_, bool absolute_path_) :
  program(program_),
  absolute_path(absolute_path_),
  arguments(),
  stdout_vec(),
  stderr_vec(),
  stdin_data()
{
}

Exec&
Exec::arg(const std::string& argument)
{
  arguments.push_back(argument);
  return *this;
}

void
Exec::set_stdin(const BlobHandle& blob)
{
  stdin_data = blob;
}

int
Exec::exec()
{
  int stdin_fd[2];
  int stdout_fd[2];
  int stderr_fd[2];

  if (pipe(stdout_fd) < 0 || pipe(stderr_fd) < 0 || pipe(stdin_fd) < 0)
    throw std::runtime_error("Exec: pipe failed");

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

    throw std::runtime_error("Exec: fork failed");
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
    boost::scoped_array<char*> c_arguments(new char*[arguments.size()+2]);
    c_arguments[0] = strdup(program.c_str());
    for(std::vector<std::string>::size_type i = 0; i < arguments.size(); ++i)
      c_arguments[i+1] = strdup(arguments[i].c_str());
    c_arguments[arguments.size()+1] = NULL;
      
    // Execute the program
    if (absolute_path)
      execv(c_arguments[0], c_arguments.get());
    else
      execvp(c_arguments[0], c_arguments.get());
      
    int error_code = errno;

    // Cleanup
    for(int i = 0; c_arguments[i] != NULL; ++i)
      free(c_arguments[i]);

    // execvp() only returns on failure 
    throw std::runtime_error("Exec: " + program + ": " + strerror(error_code));
  }
  else // if (pid > 0)
  { // parent
    close(stdin_fd[0]);
    close(stdout_fd[1]);
    close(stderr_fd[1]);

    int len;
    char buffer[4096];
      
    if (stdin_data)
    {
      if (write(stdin_fd[1], stdin_data->get_data(), stdin_data->size()) < 0)
      {
        throw std::runtime_error(strerror(errno));
      }
    }

    close(stdin_fd[1]);

    while((len = read(stdout_fd[0], buffer, sizeof(buffer))) > 0)
      stdout_vec.insert(stdout_vec.end(), buffer, buffer+len);

    if (len == -1)
      throw std::runtime_error("error reading stdout from xcfinfo");

    while((len = read(stderr_fd[0], buffer, sizeof(buffer))) > 0)
      stderr_vec.insert(stderr_vec.end(), buffer, buffer+len);

    if (len == -1)
      throw std::runtime_error("error reading stderr from xcfinfo");

    int child_status;
    waitpid(pid, &child_status, 0);

    // Cleanup
    close(stdout_fd[0]);
    close(stderr_fd[0]);

    return child_status;
  }
}

std::string
Exec::str() const
{
  std::ostringstream out;

  out << program << " ";

  for(std::vector<std::string>::size_type i = 0; i < arguments.size(); ++i)
    out << "'" << arguments[i] << "' ";

  return out.str();
}

/* EOF */
