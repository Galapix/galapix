/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2008 Ingo Ruhnke <grumbel@gmail.com>
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

#include <iostream>
#include <uitest/uitest.hpp>

#include "util/exec.hpp"

UITEST(Exec, cwd, "WORKINGDIR PROGRAM [ARGUMENTS]...")
{
  Exec prgn(args[1]);

  prgn.set_working_directory(args[0]);

  std::string stdin_data = "-- Stdin Test Data --\n";
  prgn.set_stdin(Blob::copy({reinterpret_cast<uint8_t const*>(stdin_data.c_str()), stdin_data.length()}));
  for(auto const& arg : rest) {
    prgn.arg(arg);
  }
  std::cout << "ExitCode: " << prgn.exec() << std::endl;

  std::cout << "### STDOUT BEGIN" << std::endl;
  std::cout << prgn.get_stdout_txt();
  std::cout << "### STDOUT END" << std::endl;
  std::cout << std::endl;
  std::cout << "### STERR BEGIN: " << std::endl;
  std::cout << prgn.get_stderr_txt();
  std::cout << "### STDERR END" << std::endl;

  std::cout << "stdout size: " << prgn.get_stdout().size() << std::endl;
  std::cout << "stderr size: " << prgn.get_stderr().size() << std::endl;
}

/* EOF */
