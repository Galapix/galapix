/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2013 Ingo Ruhnke <grumbel@gmail.com>
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
#include <logmich/log.hpp>
#include <uitest/uitest.hpp>

#include "archive/archive_thread.hpp"

UITEST(Archive, thread, "ARCHIVENAME...",
       "Produce an Extraction object")
{
  logmich::set_log_level(logmich::kDebug);

  ArchiveThread m_archive("/tmp/foobar");

  for(const auto& arg : rest)
  {
    std::cout << "requesting: " << arg << std::endl;
    m_archive.request_extraction
      (arg,
       [](Failable<ExtractionPtr> fail_or_extraction)
       {
         try
         {
           ExtractionPtr extraction = fail_or_extraction.get();
           std::cout << "extraction received: " << extraction->get_type() << std::endl;
           for(auto&& f : extraction.get()->get_filenames())
           {
             std::cout << f << std::endl;
           }
         }
         catch(const std::exception& err)
         {
           std::cout << "error: " << err.what() << std::endl;
         }
       });
  }

  std::cout << "shutdown" << std::endl;
}

/* EOF */
