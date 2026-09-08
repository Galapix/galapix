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

#include "galapix/size_probe_session.hpp"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iostream>

#include "galapix/mandelbrot_tile_provider.hpp"
#include "galapix/system.hpp"
#include "galapix/viewer.hpp"
#include "galapix/workspace.hpp"
#include "galapix/zoomify_tile_provider.hpp"
#include "util/filesystem.hpp"

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

struct OpenTiming {
  bool enabled = false;
  std::chrono::steady_clock::time_point t0{};
  std::chrono::steady_clock::time_point phase{};

  explicit OpenTiming(char const* label) {
    enabled = std::getenv("GALAPIX_OPEN_TIMING") != nullptr;
    if (!enabled) return;
    t0 = phase = std::chrono::steady_clock::now();
    std::cout << "[open-timing] start " << label << "\n";
  }

  void mark(char const* name) {
    if (!enabled) return;
    auto now = std::chrono::steady_clock::now();
    double dt = std::chrono::duration<double>(now - phase).count();
    double tot = std::chrono::duration<double>(now - t0).count();
    std::cout << "[open-timing] " << name << ": " << dt << " s (total "
              << tot << " s)\n";
    phase = now;
  }
};

} // namespace


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
  m_job_manager(opts.threads),
  m_patterns(opts.patterns)
#ifdef HAVE_THUMTOO
  , m_thumtoo()
#endif
{
  m_job_manager.start_thread();

#ifdef HAVE_THUMTOO
  // HAVE_THUMTOO builds always use thumtoo for file tiles (no cache4_tiles).
  thumtoo::image_library_init();
  {
    std::filesystem::path cache = m_opts.thumtoo_cache.empty()
                                    ? default_thumtoo_cache()
                                    : std::filesystem::path(m_opts.thumtoo_cache);
    m_thumtoo = thumtoo::Client::open(
      cache, ThumtooCallbackQueue::instance().make_executor());
    std::cout << "Using thumtoo cache: " << cache
              << " (Galapix SQLite tiles disabled)" << std::endl;
  }
#else
  std::cerr << "Warning: galapix was built without HAVE_THUMTOO; local file "
               "tiles need thumtoo. Zoomify/Mandelbrot still work.\n";
  if (m_opts.use_thumtoo) {
    std::cerr << "Warning: --thumtoo requested but this binary has no thumtoo.\n";
  }
#endif
}

TileProviderPtr
ViewerCommand::make_file_tile_provider(URL const& url)
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
  (void)url;
  return {};
}

ViewerCommand::~ViewerCommand()
{
  try
  {
    m_job_manager.abort_thread();
    m_job_manager.join_thread();
  }
  catch(std::exception const& err)
  {
    log_error(err.what());
  }
}

void
ViewerCommand::run(std::vector<URL> const& urls)
{
  OpenTiming timing("ViewerCommand::run");
  Workspace workspace;

#ifdef HAVE_THUMTOO
  // Single session for pattern + file URL size probes (async; UI opens first).
  auto size_probe = std::make_shared<SizeProbeSession>();
  workspace.set_size_probe(size_probe);
#endif

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
  timing.mark("expand");
#else
  std::vector<URL> const& work_urls = urls;
  timing.mark("expand");
#endif

  { // process all -p PATTERN options (thumtoo cache query when available)
#ifdef HAVE_THUMTOO
    if (m_thumtoo && !m_patterns.empty()) {
      constexpr int kPatternLimit = 50'000;

      auto glob_to_like = [](std::string const& glob) {
        // Legacy GLOB * / ? → SQL LIKE % / _; pass through % _ unchanged.
        std::string out;
        out.reserve(glob.size());
        for (char c : glob) {
          if (c == '*') out.push_back('%');
          else if (c == '?') out.push_back('_');
          else out.push_back(c);
        }
        return out;
      };

      auto looks_like_path_prefix = [](std::string const& p) {
        if (p.empty()) return false;
        if (p.front() == '/') return true;
        if (p.starts_with("file://")) return true;
        return false;
      };

      std::vector<thumtoo::Database::LocatorRow> locs;
      locs.reserve(256);

      for (std::string const& pat : m_patterns) {
        std::cout << "Processing pattern: '" << pat << "'" << std::endl;
        std::vector<thumtoo::Database::LocatorRow> batch;
        if (pat == "*") {
          batch = m_thumtoo->list_locators(kPatternLimit);
        } else if (looks_like_path_prefix(pat)) {
          std::string prefix = pat;
          if (prefix.starts_with("file://")) {
            prefix = prefix.substr(7);
          }
          batch = m_thumtoo->list_locators_by_outer_path_prefix(prefix, kPatternLimit);
        } else {
          batch = m_thumtoo->list_locators_like(glob_to_like(pat), kPatternLimit);
        }
        if (static_cast<int>(batch.size()) >= kPatternLimit) {
          std::cout << "  (limit " << kPatternLimit
                    << " reached; results may be truncated)\n";
        }
        locs.insert(locs.end(), batch.begin(), batch.end());
      }

      // Queue size probes without blocking; attach providers as sizes arrive
      // (SizeProbeSession + viewer tick). Opens the UI immediately.
      size_t added = 0;
      size_t skipped = 0;
      size_t const total = locs.size();
      for (size_t i = 0; i < total; ++i) {
        auto const& row = locs[i];
        auto url_opt = url_from_thumtoo_uri(row.uri);
        if (!url_opt || row.uri.empty()) {
          ++skipped;
        } else {
          TileProviderPtr provider;
          if (auto sz = m_thumtoo->get_size(row.uri)) {
            provider = ThumtooTileProvider::create_from_size(
              m_thumtoo, row.uri, sz->width, sz->height);
          }
          auto image = std::make_shared<Image>(
            *url_opt, provider, &m_job_manager);
          workspace.add_image(image);
          ++added;
          if (!provider) {
            m_thumtoo->request_size(row.uri, [](std::string, std::optional<thumtoo::Size>) {});
            size_probe->add_pending(image, row.uri);
          }
        }
        std::cout << "Pattern match: " << (i + 1) << "/" << total
                  << " (" << added << " images)"
                  << " - " << (total == 0 ? 0 : 100 * (i + 1) / total) << '%'
                  << '\r' << std::flush;
      }

      if (total != 0) {
        std::cout << std::endl;
      }
      std::cout << "Pattern: " << added << " image(s) from thumtoo cache"
                << (skipped ? (", " + std::to_string(skipped) + " skipped") : "")
                << std::endl;
    } else
#endif
    if (!m_patterns.empty()) {
      // Resource-DB path is stubbed (get_old_file_entries empty). Require thumtoo.
      std::cerr << "Warning: -p/--pattern needs thumtoo (HAVE_THUMTOO and a "
                   "populated cache). Resource DB pattern lookup was removed.\n";
    }
  }

#ifdef HAVE_THUMTOO
  // Queue size probes without blocking the viewer launch. Known sizes get a
  // provider immediately; the rest attach on the main thread as get_size()
  // fills in (SizeProbeSession + Viewer::update tick).
  timing.mark("size_probe_queued");
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
#ifdef HAVE_THUMTOO
      if (m_thumtoo) {
        std::string const uri = thumtoo_uri_from_url(*i);
        TileProviderPtr provider;
        if (!uri.empty()) {
          if (auto sz = m_thumtoo->get_size(uri)) {
            provider = ThumtooTileProvider::create_from_size(
              m_thumtoo, uri, sz->width, sz->height);
          }
        }
        auto image = std::make_shared<Image>(*i, provider, &m_job_manager);
        workspace.add_image(image);
        if (!provider && !uri.empty()) {
          m_thumtoo->request_size(uri, [](std::string, std::optional<thumtoo::Size>) {});
          size_probe->add_pending(image, uri);
        }
      } else
#endif
      {
        workspace.add_image(std::make_shared<Image>(*i, TileProviderPtr{}, &m_job_manager));
      }
    }
  }

#ifdef HAVE_THUMTOO
  if (m_thumtoo && size_probe->total() > 0) {
    size_probe->start_drain(m_thumtoo, size_probe);
  } else if (m_thumtoo) {
    // No outstanding probes; drop empty session.
    workspace.set_size_probe({});
  }
  timing.mark("size_probe");
#endif


  if (!urls.empty())
  {
    std::cout << std::endl;
  }

  timing.mark("add_images");
  log_info("launching viewer");
  m_system.launch_viewer(workspace, m_opts);
  // Whole interactive session until quit — not first-paint. See frame logs.
  timing.mark("viewer_session_until_quit");
  log_info("viewer done");
}

} // namespace galapix

/* EOF */
