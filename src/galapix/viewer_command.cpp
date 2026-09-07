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

#include <filesystem>
#include <iostream>

#include "galapix/database_tile_provider.hpp"
#include "database/entries/old_file_entry.hpp"
#include "database/entries/image_entry.hpp"
#include "galapix/mandelbrot_tile_provider.hpp"
#include "galapix/system.hpp"
#include "galapix/viewer.hpp"
#include "galapix/workspace.hpp"
#include "galapix/zoomify_tile_provider.hpp"

#ifdef HAVE_THUMTOO
#  include "thumtoo/thumtoo_callback_queue.hpp"
#  include "thumtoo/thumtoo_tile_provider.hpp"
#  include "thumtoo/thumtoo_uri.hpp"
#  include <thumtoo/archive.hpp>
#  include <thumtoo/client.hpp>
#  include <thumtoo/image.hpp>
#  include <thumtoo/pdf.hpp>
#  include <cstdlib>
#  include <algorithm>
#endif

namespace galapix {

namespace {

#ifdef HAVE_THUMTOO
std::filesystem::path default_thumtoo_cache()
{
  if (char const* xdg = std::getenv("XDG_CACHE_HOME"); xdg && *xdg) {
    return std::filesystem::path(xdg) / "thumtoo";
  }
  if (char const* home = std::getenv("HOME"); home && *home) {
    return std::filesystem::path(home) / ".cache" / "thumtoo";
  }
  return std::filesystem::path(".cache") / "thumtoo";
}

/** Expand a filesystem URL into Galapix URLs that map 1:1 to thumtoo media.
 *  Plain images → self. Archives → one URL per image member. PDFs → one URL
 *  per page (//page:N). Non-stdio / unknown → original URL only.
 *
 *  Uses is_likely_* helpers (available on older thumtoo); no format.hpp required.
 */
std::vector<URL> expand_thumtoo_urls(URL const& url)
{
  if (!url.has_stdio_name()) {
    return {url};
  }
  std::filesystem::path const path =
    std::filesystem::absolute(url.get_stdio_name());
  if (!std::filesystem::is_regular_file(path)) {
    return {url};
  }

  if (thumtoo::is_likely_pdf_path(path)) {
    auto pages = thumtoo::pdf_page_count(path);
    if (!pages || *pages < 1) {
      return {url};
    }
    std::vector<URL> out;
    int const n = std::min(*pages, 512);
    out.reserve(static_cast<size_t>(n));
    for (int page = 1; page <= n; ++page) {
      std::string s =
        "file://" + path.string() + "//page:" + std::to_string(page);
      out.push_back(URL::from_string(s));
    }
    return out;
  }

  if (thumtoo::is_likely_archive_path(path)) {
    auto toc = thumtoo::read_archive_toc(path);
    if (!toc) {
      return {url};
    }
    std::vector<URL> out;
    for (auto const& mem : *toc) {
      if (!thumtoo::is_likely_image_member_path(mem.member_path)) {
        continue;
      }
      std::string s =
        "file://" + path.string() + "//archive:" + mem.member_path;
      out.push_back(URL::from_string(s));
    }
    return out.empty() ? std::vector<URL>{url} : out;
  }

  return {url};
}
#endif

} // namespace

ViewerCommand::ViewerCommand(System& system, Options const& opts) :
  m_system(system),
  m_opts(opts),
  // Pure thumtoo view: keep resource DB for -p patterns, skip Galapix tile SQLite.
  // Only when HAVE_THUMTOO and use_thumtoo; otherwise keep historical tile DB.
  m_database(Database::create(
    opts.database,
#ifdef HAVE_THUMTOO
    /*sqlite_tiles=*/!opts.use_thumtoo
#else
    /*sqlite_tiles=*/true
#endif
  )),
  m_job_manager(opts.threads),
  m_database_thread(m_database, m_job_manager),
  m_patterns(opts.patterns)
{
  m_job_manager.start_thread();
  m_database_thread.start_thread();

#ifdef HAVE_THUMTOO
  if (m_opts.use_thumtoo) {
    thumtoo::image_library_init();
    std::filesystem::path cache = m_opts.thumtoo_cache.empty()
                                    ? default_thumtoo_cache()
                                    : std::filesystem::path(m_opts.thumtoo_cache);
    m_thumtoo = thumtoo::Client::open(
      cache, ThumtooCallbackQueue::instance().make_executor());
    std::cout << "Using thumtoo cache: " << cache
              << " (Galapix SQLite tiles disabled)" << std::endl;
  }
#else
  if (m_opts.use_thumtoo) {
    std::cerr << "Warning: --thumtoo requested but galapix was built without "
                 "HAVE_THUMTOO; using SQLite tiles.\n";
  }
#endif
}

TileProviderPtr
ViewerCommand::make_file_tile_provider(URL const& url,
                                       OldFileEntry const* file_entry,
                                       ImageEntry const* image_entry)
{
#ifdef HAVE_THUMTOO
  if (m_thumtoo) {
    std::string const uri = thumtoo_uri_from_url(url);
    if (uri.empty()) {
      log_warn("thumtoo: cannot map URL to URI: {}", url);
    } else if (auto sz = m_thumtoo->get_size(uri)) {
      if (auto p = ThumtooTileProvider::create_from_size(
            m_thumtoo, uri, sz->width, sz->height)) {
        return p;
      }
      log_warn("thumtoo provider failed for {}; no SQLite tile fallback", uri);
    } else if (auto p = ThumtooTileProvider::create(m_thumtoo, uri)) {
      // Missed batch probe (or single-file path).
      return p;
    } else {
      log_warn("thumtoo provider failed for {}; no SQLite tile fallback", uri);
    }
    return {};
  }
#endif
  if (file_entry && image_entry) {
    return std::make_shared<DatabaseTileProvider>(*file_entry, *image_entry);
  }
  return {};
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
  catch(std::exception const& err)
  {
    log_error(err.what());
  }
}

void
ViewerCommand::run(std::vector<URL> const& urls)
{
  Workspace workspace;

#ifdef HAVE_THUMTOO
  // Expand PDF pages and archive image members into per-item URLs so the rest
  // of the pipeline stays "one Image ↔ one thumtoo URI".
  std::vector<URL> expanded_urls;
  if (m_thumtoo) {
    expanded_urls.reserve(urls.size());
    for (URL const& u : urls) {
      auto parts = expand_thumtoo_urls(u);
      expanded_urls.insert(expanded_urls.end(), parts.begin(), parts.end());
    }
    if (expanded_urls.size() != urls.size()) {
      std::cout << "Expanded " << urls.size() << " path(s) -> "
                << expanded_urls.size()
                << " media item(s) (archives/PDF)\n";
    }
  }
  std::vector<URL> const& work_urls = m_thumtoo ? expanded_urls : urls;
#else
  std::vector<URL> const& work_urls = urls;
#endif

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
        workspace.add_image(std::make_shared<Image>(i->get_url(),
                                          make_file_tile_provider(i->get_url(), &*i, &image_entry),
                                          &m_job_manager));

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

#ifdef HAVE_THUMTOO
  // Batch thumtoo size probes: one drain for the whole list. Per-file
  // create()+drain was O(n) sequential open/hash/decode and dominated
  // "Processing URLs" for large folders (not Galapix SHA1).
  if (m_thumtoo) {
    std::cout << "Probing image sizes (thumtoo)..." << std::flush;
    int pending = 0;
    for (URL const& u : work_urls) {
      if (u.get_protocol() == "builtin") continue;
      if (Filesystem::has_extension(u.str(), "ImageProperties.xml")) continue;
      if (u.has_stdio_name() && Filesystem::has_extension(u.get_stdio_name(), ".galapix")) continue;
      std::string const uri = thumtoo_uri_from_url(u);
      if (uri.empty()) continue;
      if (m_thumtoo->get_size(uri)) continue;
      ++pending;
      m_thumtoo->request_size(uri, [](std::string, std::optional<thumtoo::Size>) {});
    }
    if (pending > 0) {
      m_thumtoo->drain();
      ThumtooCallbackQueue::instance().pump();
    }
    std::cout << " " << work_urls.size() << " urls, " << pending << " probed\n";
  }
#endif

  // process regular URLs
  for(std::vector<URL>::const_iterator i = work_urls.begin(); i != work_urls.end(); ++i)
  {
    size_t n = static_cast<size_t>(i - work_urls.begin()) + 1;
    size_t total = work_urls.size();
    std::cout << "Processing URLs: " << n << "/" << total << " - " << (100 * n / total) << "%\r" << std::flush;

    if (i->has_stdio_name() && Filesystem::has_extension(i->get_stdio_name(), ".galapix"))
    {
      // FIXME: Right place for this?
      workspace.load(i->get_stdio_name());
    }
    else if (i->get_protocol() == "builtin")
    {
      if (i->get_payload() == "mandelbrot")
      {
        workspace.add_image(std::make_shared<Image>(*i, std::make_shared<MandelbrotTileProvider>(m_job_manager), &m_job_manager));
      }
      else
      {
        std::cout << "Galapix::view(): unknown builtin:// requested: " << *i << " ignoring" << std::endl;
      }
    }
    else if (Filesystem::has_extension(i->str(), "ImageProperties.xml"))
    {
      workspace.add_image(std::make_shared<Image>(*i, ZoomifyTileProvider::create(*i, m_job_manager), &m_job_manager));
    }
    else
    {
      OldFileEntry file_entry;
      if (!m_database.get_resources().get_old_file_entry(*i, file_entry))
      {
        if (auto provider = make_file_tile_provider(*i)) {
          workspace.add_image(std::make_shared<Image>(*i, provider, &m_job_manager));
        } else {
          workspace.add_image(std::make_shared<Image>(*i, TileProviderPtr{}, &m_job_manager));
        }
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
          workspace.add_image(std::make_shared<Image>(
            file_entry.get_url(),
            make_file_tile_provider(file_entry.get_url(), &file_entry, &image_entry),
            &m_job_manager));
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

} // namespace galapix

/* EOF */
