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

#include "galapix/viewer_command.hpp"

#include "galapix/database_tile_provider.hpp"
#include "galapix/mandelbrot_tile_provider.hpp"
#include "galapix/system.hpp"
#include "galapix/viewer.hpp"
#include "galapix/workspace.hpp"
#include "galapix/zoomify_tile_provider.hpp"

ViewerCommand::ViewerCommand(System& system, const Options& opts) :
  m_system(system),
  m_opts(opts),
  m_database(Database::create(opts.database)),
  m_job_manager(opts.threads),
  m_database_thread(m_database, m_job_manager),
  m_patterns(opts.patterns)
{
  m_job_manager.start_thread();
  m_database_thread.start_thread();
}

ViewerCommand::~ViewerCommand()
{
  try
  {
    m_job_manager.abort_thread();
    m_database_thread.abort_thread();

    m_job_manager.join_thread();
    m_database_thread.join_thread();
  }
  catch(const std::exception& err)
  {
    log_error(err.what());
  }
}

void
ViewerCommand::run(const std::vector<URL>& urls)
{
  Workspace workspace;

  { // process all -p PATTERN options
    std::vector<OldFileEntry> file_entries;

    for(std::vector<std::string>::const_iterator i = m_patterns.begin(); i != m_patterns.end(); ++i)
    {
      std::cout << "Processing pattern: '" << *i << "'" << std::endl;

      if (*i == "*")
      {
        // special case to display everything, might be faster then
        // using the pattern
        m_database.get_resources().get_old_file_entries(file_entries);
      }
      else
      {
        m_database.get_resources().get_old_file_entries(*i, file_entries);
      }
    }

    for(std::vector<OldFileEntry>::const_iterator i = file_entries.begin(); i != file_entries.end(); ++i)
    {
      ImageEntry image_entry;
      if (!m_database.get_resources().get_image_entry(*i, image_entry))
      {
        log_warn("no ImageEntry for {}", i->get_url());
      }
      else
      {
        workspace.add_image(std::make_shared<Image>(i->get_url(), std::make_shared<DatabaseTileProvider>(*i, image_entry)));

        // print progress
        size_t n = static_cast<size_t>(i - file_entries.begin()) + 1;
        size_t total = file_entries.size();
        std::cout << "Getting tiles: " << n << "/" << total << " - "
                  << (100 * n / total) << '%'
                  << '\r' << std::flush;
      }
    }

    if (!file_entries.empty())
    {
      std::cout << std::endl;
    }
  }

  // process regular URLs
  for(std::vector<URL>::const_iterator i = urls.begin(); i != urls.end(); ++i)
  {
    size_t n = static_cast<size_t>(i - urls.begin()) + 1;
    size_t total = urls.size();
    std::cout << "Processing URLs: " << n << "/" << total << " - " << (100 * n / total) << "%\r" << std::flush;

    if (i->has_stdio_name() && Filesystem::has_extension(i->get_stdio_name(), ".galapix"))
    {
      // FIXME: Right place for this?
      workspace.load(i->get_stdio_name());
    }
    else if (i->get_protocol() == "buildin")
    {
      if (i->get_payload() == "mandelbrot")
      {
        workspace.add_image(std::make_shared<Image>(*i, std::make_shared<MandelbrotTileProvider>(m_job_manager)));
      }
      else
      {
        std::cout << "Galapix::view(): unknown buildin:// requested: " << *i << " ignoring" << std::endl;
      }
    }
    else if (Filesystem::has_extension(i->str(), "ImageProperties.xml"))
    {
      workspace.add_image(std::make_shared<Image>(*i, ZoomifyTileProvider::create(*i, m_job_manager)));
    }
    else
    {
      OldFileEntry file_entry;
      if (!m_database.get_resources().get_old_file_entry(*i, file_entry))
      {
        workspace.add_image(std::make_shared<Image>(*i));
      }
      else
      {
        ImageEntry image_entry;
        if (!m_database.get_resources().get_image_entry(file_entry, image_entry))
        {
          log_warn("no ImageEntry for {}", *i);
        }
        else
        {
          workspace.add_image(std::make_shared<Image>(file_entry.get_url(),
                                                      std::make_shared<DatabaseTileProvider>(file_entry, image_entry)));
        }
      }
    }
  }

  if (!urls.empty())
  {
    std::cout << std::endl;
  }

  log_info("launching viewer");
  m_system.launch_viewer(workspace, m_opts);
  log_info("viewer done");
}

/* EOF */
