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

#include <iostream>
#include <errno.h>
#include <sstream>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "util/log.hpp"
#include "util/raise_exception.hpp"

Exec::Exec(const std::string& program, bool absolute_path) :
  m_program(program),
  m_absolute_path(absolute_path),
  m_arguments(),
  m_stdout_vec(),
  m_stderr_vec(),
  m_stdin_data()
{}

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

  // FIXME: Bad, we potentially leak file descriptors
  if (pipe(stdout_fd) < 0 || pipe(stderr_fd) < 0 || pipe(stdin_fd) < 0)
    raise_runtime_error("Exec:exec(): pipe failed");

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

    raise_runtime_error("Exec::exec(): fork failed");
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
    std::unique_ptr<char*[]> c_arguments(new char*[m_arguments.size()+2]);
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

    // FIXME: this ain't proper, need to exit(1) on failure and signal error to parent somehow

    // execvp() only returns on failure 
    std::cout << "Exec::exec(): " << m_program << ": " << strerror(error_code) << std::endl;
    _exit(EXIT_FAILURE);
  }
  else // if (pid > 0)
  { // parent
    close(stdin_fd[0]);
    close(stdout_fd[1]);
    close(stderr_fd[1]);

    try 
    {
      process_io(stdin_fd[1], stdout_fd[0], stderr_fd[0]);
    }
    catch(std::exception& err)
    {
      int child_status = 0;
      waitpid(pid, &child_status, 0);
      throw;
    }

    int child_status = 0;
    waitpid(pid, &child_status, 0);

    return WEXITSTATUS(child_status);
  }
}

void
Exec::process_io(int stdin_fd, int stdout_fd, int stderr_fd)
{
  char buffer[4096];
      
  // write data to stdin
  if (m_stdin_data)
  {
    if (write(stdin_fd, m_stdin_data->get_data(), m_stdin_data->size()) < 0)
    {
      close(stdin_fd);
      close(stdout_fd);
      close(stderr_fd);

      std::ostringstream out;
      out << "Exec::process_io(): stdin write failure: " << str() << ": " << strerror(errno);
      raise_runtime_error(out.str());
    }
  }
  close(stdin_fd);

  // start reading from stdout/stderr
  bool stdout_eof = false;
  bool stderr_eof = false;
  while(!(stdout_eof && stderr_eof))
  {
    fd_set rfds;   
    FD_ZERO(&rfds);

    int nfds = 0;

    if (!stdout_eof) 
    {
      FD_SET(stdout_fd, &rfds);
      nfds = std::max(nfds, stdout_fd);
    }

    if (!stderr_eof) 
    {
      FD_SET(stderr_fd, &rfds);
      nfds = std::max(nfds, stderr_fd);
    }

    int retval = select(nfds+1, &rfds, NULL, NULL, NULL);

    if (retval < 0)
    {
      close(stdout_fd);
      close(stderr_fd);

      std::ostringstream out;
      out << "Exec::process_io(): select() failure: " << str() << ": " << strerror(errno);
      raise_runtime_error(out.str());
    }
    else if (retval == 0)
    {
      log_error("select() returned without results, this shouldn't happen");
    }
    else // retval > 0
    {
      if (!stdout_eof && FD_ISSET(stdout_fd, &rfds))
      {
        ssize_t len = read(stdout_fd, buffer, sizeof(buffer));
        
        if (len < 0) // error
        {
          close(stdout_fd);
          close(stderr_fd);

          std::ostringstream out;
          out << "Exec::process_io(): stdout read failure: " << str() << ": " << strerror(errno);
          raise_runtime_error(out.str());
        }
        else if (len > 0) // ok
        {
          m_stdout_vec.insert(m_stdout_vec.end(), buffer, buffer+len);
        }
        else if (len == 0) // eof
        {
          close(stdout_fd);
          stdout_eof = true;
        }
      }

      if (!stderr_eof && FD_ISSET(stderr_fd, &rfds))
      {
        ssize_t len = read(stderr_fd, buffer, sizeof(buffer));

        if (len < 0) // error
        {
          close(stdout_fd);
          close(stderr_fd);

          std::ostringstream out;
          out << "Exec::process_io(): stderr read failure: " << str() << ": " << strerror(errno);
          raise_runtime_error(out.str());
        }
        else if (len > 0) // ok
        {
          m_stderr_vec.insert(m_stderr_vec.end(), buffer, buffer+len);
        }
        else if (len == 0) // eof
        {
          close(stderr_fd);
          stderr_eof = true;
        }
      }
    }
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
