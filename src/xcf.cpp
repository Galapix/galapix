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

#include "xcf.hpp"

std::string xcfinfo(const std::string& filename)
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

      execlp("xcfinfo", "xcfinfo", filename.c_str(), NULL);
    }
  else // if (pid > 0)
    { // parent
      close(stdout_fd[1]);
      close(stderr_fd[1]);

      int len;
      char buffer[4096];
      std::string str_stdout;
      std::string str_stderr;
      
      while((len = read(stdout_fd[0], buffer, sizeof(buffer))) > 0)
        str_stdout.append(buffer, len);

      if (len == -1)
        throw std::runtime_error("error reading stdout from xcfinfo");

      while((len = read(stderr_fd[0], buffer, sizeof(buffer))) > 0)
        str_stderr.append(buffer, len);

      if (len == -1)
        throw std::runtime_error("error reading stderr from xcfinfo");

      int child_status;
      waitpid(pid, &child_status, 0);

      std::cout << "Exit Status: " << child_status << std::endl;

      // Cleanup
      close(stdout_fd[0]);
      close(stderr_fd[0]);

      if (child_status == 0)
        return str_stdout;
      else
        return str_stderr;
    }
}

bool
XCF::get_size(const std::string& filename, Size& size)
{
  return false;
}

SoftwareSurface
XCF::load_from_file(const std::string& filename)
{
  return SoftwareSurface();
}

/* EOF */
