/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2012 Ingo Ruhnke <grumbel@gmx.de>
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

#include "generator/generator.hpp"

#include "util/log.hpp"

Generator::Generator(JobManager& job_manager) :
  m_job_manager(job_manager)
{
}

void
Generator::request_file_info(const std::string& path, 
                             const std::function<void (const Failable<FileInfo>&)>& callback)
{
  log_info(path);
  Failable<FileInfo> result;
    
  try 
  {
    log_debug("trying to get FileInfo");
    result.reset(FileInfo::from_file(path));
    log_debug("trying to get FileInfo: success");
  }
  catch(...)
  {
    log_debug("trying to get FileInfo: failed");
    result.set_exception(std::current_exception());
  }

  callback(result);
}

/* EOF */
