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
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

#include "exec.hpp"

Exec::Exec(const std::string& program)
  : program(program)
{
  
}

Exec&
Exec::arg(const std::string& argument)
{
  arguments.push_back(argument);
  return *this;
}

int
Exec::exec()
{
  int stdout_fd[2];
  int stderr_fd[2];

  if (pipe(stdout_fd) < 0)
    throw std::runtime_error("pipe failed");

  if (pipe(stderr_fd) < 0)
    throw std::runtime_error("pipe failed");

  pid_t pid = fork();
  if (pid < 0)
    { // error
      throw std::runtime_error("fork failed");
    }
  else if (pid == 0) 
    { // child
      close(stdout_fd[0]);
      close(stderr_fd[0]);
      dup2(stdout_fd[1], STDOUT_FILENO);
      dup2(stderr_fd[1], STDERR_FILENO);
      close(stdout_fd[1]); // 
      close(stderr_fd[1]); // 

      const char* c_arguments[arguments.size()+2];
      c_arguments[0] = program.c_str();
      for(std::vector<std::string>::size_type i = 0; i < arguments.size(); ++i)
        c_arguments[i+1] = arguments[i].c_str();
      c_arguments[arguments.size()+1] = NULL;
      return execvp(c_arguments[0], const_cast<char**>(c_arguments));
    }
  else // if (pid > 0)
    { // parent
      close(stdout_fd[1]);
      close(stderr_fd[1]);

      int len;
      char buffer[4096];
      
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

/* EOF */
