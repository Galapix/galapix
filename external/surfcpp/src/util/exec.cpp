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

#include "util/exec.hpp"

#include <array>
#include <format>
#include <iostream>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <sstream>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <logmich/log.hpp>

namespace surf {

Exec::Exec(const std::string& program, bool absolute_path) :
  m_program(program),
  m_absolute_path(absolute_path),
  m_arguments(),
  m_working_directory(),
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
Exec::set_working_directory(const std::string& path)
{
  m_working_directory = path;
}

void
Exec::set_stdin(std::vector<uint8_t> blob)
{
  m_stdin_data = std::move(blob);
}

int
Exec::exec()
{
  int stdin_fd[2];
  int stdout_fd[2];
  int stderr_fd[2];

  if (pipe(stdout_fd) < 0) {
    throw std::runtime_error(std::format("Exec::exec(): pipe failed: {}", strerror(errno)));
  }

  if (pipe(stderr_fd) < 0) {
    int const errnum = errno;
    close(stdout_fd[0]);
    close(stdout_fd[1]);
    throw std::runtime_error(std::format("Exec::exec(): pipe failed: {}", strerror(errnum)));
  }

  if (pipe(stdin_fd) < 0) {
    int const errnum = errno;
    close(stdout_fd[0]);
    close(stdout_fd[1]);
    close(stderr_fd[0]);
    close(stderr_fd[1]);
    throw std::runtime_error(std::format("Exec::exec(): pipe failed: {}", strerror(errnum)));
  }

  pid_t pid = fork();
  if (pid < 0)
  { // error
    int errnum = errno;

    // Cleanup
    close(stdout_fd[0]);
    close(stdout_fd[1]);
    close(stderr_fd[0]);
    close(stderr_fd[1]);
    close(stdin_fd[0]);
    close(stdin_fd[1]);

    throw std::runtime_error(std::format("Exec::exec(): fork failed: {}", strerror(errnum)));
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
    std::vector<char*> c_arguments(m_arguments.size() + 2);
    c_arguments[0] = strdup(m_program.c_str());
    for(std::vector<std::string>::size_type i = 0; i < m_arguments.size(); ++i) {
      c_arguments[i+1] = strdup(m_arguments[i].c_str());
    }

    c_arguments[m_arguments.size()+1] = nullptr;

    if (m_working_directory)
    {
      if (chdir(m_working_directory->c_str()) != 0)
      {
        int errnum = errno;
        log_error("{}: failed change to directory: {}", *m_working_directory, strerror(errnum));
        _exit(EXIT_FAILURE);
      }
    }

    // Execute the program
    if (m_absolute_path)
    {
      execv(c_arguments[0], c_arguments.data());
    }
    else
    {
      execvp(c_arguments[0], c_arguments.data());
    }

    int error_code = errno;

    // FIXME: this ain't proper, need to exit(1) on failure and signal error to parent somehow

    // execvp() only returns on failure
    log_error("{}: {}", m_program, strerror(error_code));
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
    while (waitpid(pid, &child_status, 0) < 0) {
      if (errno != EINTR) {
        throw std::runtime_error(std::format("Exec::exec(): waitpid failed: {}: {}", str(), strerror(errno)));
      }
    }

    if (WIFEXITED(child_status)) {
      return WEXITSTATUS(child_status);
    } else if (WIFSIGNALED(child_status)) {
      throw std::runtime_error(std::format("Exec::exec(): {} killed by signal {}", str(), WTERMSIG(child_status)));
    } else {
      throw std::runtime_error(std::format("Exec::exec(): {} terminated abnormally", str()));
    }
  }
}

namespace {

/** Block SIGPIPE for the current thread, so that a child that exits
    without reading all of stdin doesn't kill us, write() will return
    EPIPE instead */
class SigpipeBlocker
{
public:
  SigpipeBlocker() :
    m_mask(),
    m_old_mask()
  {
    sigemptyset(&m_mask);
    sigaddset(&m_mask, SIGPIPE);
    pthread_sigmask(SIG_BLOCK, &m_mask, &m_old_mask);
  }

  ~SigpipeBlocker()
  {
    if (!sigismember(&m_old_mask, SIGPIPE)) {
      // discard a SIGPIPE raised while blocked
      struct timespec const zero{0, 0};
      while (sigtimedwait(&m_mask, nullptr, &zero) > 0) {}
    }
    pthread_sigmask(SIG_SETMASK, &m_old_mask, nullptr);
  }

  SigpipeBlocker(SigpipeBlocker const&) = delete;
  SigpipeBlocker& operator=(SigpipeBlocker const&) = delete;

private:
  sigset_t m_mask;
  sigset_t m_old_mask;
};

} // namespace

void
Exec::process_io(int stdin_fd, int stdout_fd, int stderr_fd)
{
  SigpipeBlocker const sigpipe_blocker;

  auto close_fd = [](int& fd) {
    if (fd >= 0) {
      close(fd);
      fd = -1;
    }
  };

  auto fail = [&](std::string_view what) {
    int const errnum = errno;
    close_fd(stdin_fd);
    close_fd(stdout_fd);
    close_fd(stderr_fd);
    throw std::runtime_error(std::format("Exec::process_io(): {} failure: {}: {}", what, str(), strerror(errnum)));
  };

  size_t stdin_pos = 0;
  if (m_stdin_data.empty()) {
    close_fd(stdin_fd);
  } else {
    // stdin is written while stdout/stderr are read, as the child
    // might block on writing output before it has consumed all input
    int const flags = fcntl(stdin_fd, F_GETFL);
    if (flags < 0 || fcntl(stdin_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
      fail("fcntl()");
    }
  }

  auto read_fd = [&](int& fd, std::vector<char>& out, std::string_view name) {
    char buffer[4096];
    ssize_t const len = read(fd, buffer, sizeof(buffer));
    if (len < 0) {
      if (errno != EINTR && errno != EAGAIN) {
        fail(name);
      }
    } else if (len == 0) {
      close_fd(fd);
    } else {
      out.insert(out.end(), buffer, buffer + len);
    }
  };

  while (stdin_fd >= 0 || stdout_fd >= 0 || stderr_fd >= 0)
  {
    // a negative fd is ignored by poll()
    std::array<pollfd, 3> fds{{
        { stdin_fd, POLLOUT, 0 },
        { stdout_fd, POLLIN, 0 },
        { stderr_fd, POLLIN, 0 }
      }};

    if (poll(fds.data(), fds.size(), -1) < 0) {
      if (errno == EINTR) {
        continue;
      }
      fail("poll()");
    }

    if (stdin_fd >= 0 && fds[0].revents != 0) {
      ssize_t const len = write(stdin_fd, m_stdin_data.data() + stdin_pos, m_stdin_data.size() - stdin_pos);
      if (len < 0) {
        if (errno == EPIPE) {
          // child closed stdin, discard the remaining data
          close_fd(stdin_fd);
        } else if (errno != EINTR && errno != EAGAIN) {
          fail("stdin write");
        }
      } else {
        stdin_pos += static_cast<size_t>(len);
        if (stdin_pos == m_stdin_data.size()) {
          close_fd(stdin_fd);
        }
      }
    }

    if (stdout_fd >= 0 && fds[1].revents != 0) {
      read_fd(stdout_fd, m_stdout_vec, "stdout read");
    }

    if (stderr_fd >= 0 && fds[2].revents != 0) {
      read_fd(stderr_fd, m_stderr_vec, "stderr read");
    }
  }
}

std::string
Exec::str() const
{
  std::ostringstream out;

  out << m_program << " ";

  for(std::vector<std::string>::size_type i = 0; i < m_arguments.size(); ++i) {
    out << "'" << m_arguments[i] << "' ";
  }

  return out.str();
}

} // namespace surf

/* EOF */
