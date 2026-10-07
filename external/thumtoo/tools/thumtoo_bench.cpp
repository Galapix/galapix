// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Cold-cache phase timing of the real thumtoo pipeline (Client, store, worker
// pool). The golden-path tools (thumtoo-gp-*) measure the libraries alone;
// this one measures what thumtoo makes of them. See --help.

#include "thumtoo/build_stats.hpp"
#include "thumtoo/client.hpp"
#include "thumtoo/image.hpp"
#include "thumtoo/uri.hpp"
#include "thumtoo/version.hpp"

#include "golden/gp_cli.hpp"
#include "golden/gp_json.hpp"
#include "golden/gp_output.hpp"

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace {

namespace fs = std::filesystem;

fs::path default_cache_root() {
  if (const char* xdg = std::getenv("XDG_CACHE_HOME"); xdg && *xdg) {
    return fs::path(xdg) / "thumtoo-bench";
  }
  if (const char* home = std::getenv("HOME"); home && *home) {
    return fs::path(home) / ".cache" / "thumtoo-bench";
  }
  return fs::path(".cache") / "thumtoo-bench";
}

double wall_s_now(std::chrono::steady_clock::time_point t0) {
  return std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
}

/// A phase's BuildStats counters. The *_s values are summed thread time, so
/// with several workers they can exceed the phase's wall time.
struct Counters {
  double archive_extract_s = 0, image_load_s = 0, shrink_s = 0;
  double jpeg_encode_s = 0, thumb_s = 0, jxl_encode_s = 0;
  std::uint64_t levels_encoded = 0, tiles_encoded = 0, archive_bytes = 0;
  std::uint64_t exif_thumb_hits = 0, probes_done = 0, pixel_jobs = 0, tile_jobs = 0;
};

Counters snapshot_counters() {
  const thumtoo::BuildStats& s = thumtoo::global_build_stats();
  const auto sec = [](const std::atomic<std::uint64_t>& ns) {
    return static_cast<double>(ns.load(std::memory_order_relaxed)) / 1e9;
  };
  const auto n = [](const std::atomic<std::uint64_t>& v) { return v.load(std::memory_order_relaxed); };
  Counters c;
  c.archive_extract_s = sec(s.archive_extract_ns);
  c.image_load_s = sec(s.image_load_ns);
  c.shrink_s = sec(s.shrink_ns);
  c.jpeg_encode_s = sec(s.jpeg_encode_ns);
  c.thumb_s = sec(s.thumb_ns);
  c.jxl_encode_s = sec(s.jxl_encode_ns);
  c.levels_encoded = n(s.levels_encoded);
  c.tiles_encoded = n(s.tiles_encoded);
  c.archive_bytes = n(s.archive_bytes);
  c.exif_thumb_hits = n(s.exif_thumb_hits);
  c.probes_done = n(s.probes_done);
  c.pixel_jobs = n(s.pixel_jobs);
  c.tile_jobs = n(s.tile_jobs);
  return c;
}

struct PhaseResult {
  std::string name;
  double wall_s = 0;
  std::string pretty;  // BuildStats::summary_pretty()
  Counters counters;
};

struct RunInfo {
  fs::path cache;
  bool keep_cache = false;
  std::size_t paths = 0;
  std::size_t uris = 0;
  unsigned workers = 0;
  double total_wall_s = 0;
};

// --- output ---------------------------------------------------------------------

/// Keys of schema 1 are unchanged; "counters" per phase is additive and
/// carries the numbers that "stats" holds as pre-formatted text.
void write_json(const RunInfo& run, const std::vector<PhaseResult>& phases) {
  gp::JsonWriter w(std::cout);
  w.begin_object();
  w.field("schema", 1);
  w.field("tool", "thumtoo-bench");
  w.field("cache", run.cache.string());
  w.field("keep_cache", run.keep_cache);
  w.field("paths", run.paths);
  w.field("uris", run.uris);
  w.field("workers", run.workers);
  w.field("total_wall_s", run.total_wall_s);
  w.key("phases").begin_array();
  for (const PhaseResult& ph : phases) {
    w.begin_object();
    w.field("name", ph.name);
    w.field("wall_s", ph.wall_s);
    w.field("stats", ph.pretty);
    const Counters& c = ph.counters;
    w.key("counters").begin_object();
    w.field("archive_extract_s", c.archive_extract_s);
    w.field("image_load_s", c.image_load_s);
    w.field("shrink_s", c.shrink_s);
    w.field("jpeg_encode_s", c.jpeg_encode_s);
    w.field("thumb_s", c.thumb_s);
    w.field("jxl_encode_s", c.jxl_encode_s);
    w.field("levels_encoded", c.levels_encoded);
    w.field("tiles_encoded", c.tiles_encoded);
    w.field("archive_bytes", c.archive_bytes);
    w.field("exif_thumb_hits", c.exif_thumb_hits);
    w.field("probes_done", c.probes_done);
    w.field("pixel_jobs", c.pixel_jobs);
    w.field("tile_jobs", c.tile_jobs);
    w.end_object();
    w.end_object();
  }
  w.end_array();
  w.end_object();
}

/// One row per phase, plus a "total" row (wall time of the whole run, which
/// also covers opening the client). Counters are empty on the total row.
void write_csv(const RunInfo& run, const std::vector<PhaseResult>& phases, bool header) {
  gp::CsvWriter csv(std::cout,
                    {"phase", "wall_s", "workers", "images", "archive_extract_s", "image_load_s",
                     "shrink_s", "jpeg_encode_s", "thumb_s", "jxl_encode_s", "levels_encoded",
                     "tiles_encoded", "archive_bytes", "exif_thumb_hits", "probes_done",
                     "pixel_jobs", "tile_jobs"},
                    header);
  for (const PhaseResult& ph : phases) {
    const Counters& c = ph.counters;
    csv.row({ph.name, gp::csv_num(ph.wall_s), std::to_string(run.workers),
             std::to_string(run.uris), gp::csv_num(c.archive_extract_s),
             gp::csv_num(c.image_load_s), gp::csv_num(c.shrink_s), gp::csv_num(c.jpeg_encode_s),
             gp::csv_num(c.thumb_s), gp::csv_num(c.jxl_encode_s), std::to_string(c.levels_encoded),
             std::to_string(c.tiles_encoded), std::to_string(c.archive_bytes),
             std::to_string(c.exif_thumb_hits), std::to_string(c.probes_done),
             std::to_string(c.pixel_jobs), std::to_string(c.tile_jobs)});
  }
  csv.row({"total", gp::csv_num(run.total_wall_s), std::to_string(run.workers),
           std::to_string(run.uris), "", "", "", "", "", "", "", "", "", "", "", "", ""});
}

void write_text(const RunInfo& run, const std::vector<PhaseResult>& phases) {
  std::cout << "thumtoo-bench  " << run.paths << (run.paths == 1 ? " path" : " paths") << ", "
            << run.uris << (run.uris == 1 ? " image" : " images") << ", " << run.workers
            << (run.workers == 1 ? " worker" : " workers") << "\n"
            << "cache " << run.cache.string()
            << (run.keep_cache ? " (kept: warm run)" : " (wiped before the run)") << "\n\n";

  gp::TextTable table({{"phase", gp::Align::Left}, {"wall s", gp::Align::Right}});
  for (const PhaseResult& ph : phases) table.add_row({ph.name, gp::fmt_fixed(ph.wall_s, 3)});
  table.add_row({"total", gp::fmt_fixed(run.total_wall_s, 3)});
  table.print(std::cout, "");

  for (const PhaseResult& ph : phases) {
    std::cout << "\n▸ phase " << ph.name << "  wall=" << gp::fmt_fixed(ph.wall_s, 3) << " s\n"
              << ph.pretty << "\n";
  }
}

// --- command line ---------------------------------------------------------------

gp::cli::Spec make_spec() {
  gp::cli::Spec s;
  s.program = "thumtoo-bench";
  s.synopsis = "[OPTION]... PATH...";
  s.summary =
      "Time thumtoo's real pipeline (client, store and worker pool) on files, archives "
      "or directories, phase by phase, starting from an empty cache. Each phase gets "
      "a fresh statistics window; the whole run's wall time is reported as well. The "
      "golden-path tools (thumtoo-gp-archive, -gp-tile, -microbench-decode) measure the "
      "underlying libraries alone; this shows what thumtoo makes of them.";
  s.options = {
      {"cache", 0, "DIR",
       "Cache root, user data included; wiped at the start unless --keep-cache (default: "
       "$XDG_CACHE_HOME/thumtoo-bench or ~/.cache/thumtoo-bench).", "Run"},
      {"keep-cache", 0, "", "Do not wipe the cache first: measures the warm path.", "Run"},
      {"jobs", 'j', "N", "Worker threads, 0-256; 0 = one per CPU (default).", "Run"},
      {"no-probe", 0, "",
       "Skip reporting the size-probe phase. The probe still runs when a later phase "
       "needs its results.", "Phases"},
      {"ladder", 0, "EDGE",
       "Preview phase: long edge in pixels, 1-65535 (default 256); 0 skips the phase.",
       "Phases"},
      {"tile-cell", 0, "", "Add a phase that requests one tile per image (see --cell).",
       "Phases"},
      {"cell", 0, "SCALE,X,Y",
       "Which tile --tile-cell requests (default 0,0,0). Implies --tile-cell.", "Phases"},
      {"tiles", 0, "", "Add a phase that builds the tile pyramid of every image.", "Phases"},
      {"min-scale", 0, "N",
       "Finest pyramid scale for --tiles, 0-30 (default 0). Implies --tiles.", "Phases"},
      {"max-scale", 0, "M",
       "Coarsest pyramid scale for --tiles, 0-30, or -1 for all (default -1). Implies "
       "--tiles.", "Phases"},
  };
  for (auto o : gp::output_options()) {
    o.section = "Output";
    s.options.push_back(o);
  }
  s.sections = {
      {"Phases",
       "Run in this order; each needs the size probe's results.\n"
       "1. probe      size probe of every image\n"
       "2. preview    one preview per image (--ladder EDGE)\n"
       "3. tile-cell  one tile per image (--tile-cell / --cell)\n"
       "4. tiles      full tile pyramids (--tiles, --min-scale, --max-scale)"},
      {"Notes",
       "Archives, especially solid RAR/CBR: one sequential pass extracts the\n"
       "members; encoding then parallelizes across --jobs. Solid RAR cannot seek,\n"
       "so wall time follows decompression and disk more than CPU.\n"
       "--tiles builds complete pyramids (scale 0 and up) for every image; for a\n"
       "quick check prefer --tile-cell or --min-scale 2.\n"
       "Counters in the per-phase statistics are summed thread time: with several\n"
       "workers they can exceed the phase's wall time."},
      {"Output formats",
       "Default: a phase table, then the statistics of each phase.\n"
       "--csv:   one row per phase plus a total row; counters in seconds / counts.\n"
       "--json:  one document; per phase the formatted statistics (\"stats\") and\n"
       "         the same numbers as \"counters\".\n"
       "Warnings and errors go to stderr, so stdout is clean for pipes."},
      {"Exit status",
       "0  success\n"
       "1  no image could be read from the PATHs, or the run failed\n"
       "2  usage error"},
      {"Examples",
       "thumtoo-bench photo.jpg\n"
       "thumtoo-bench --jobs 4 --tile-cell comics/*.cbz\n"
       "thumtoo-bench --min-scale 2 --max-scale 4 ~/Pictures\n"
       "thumtoo-bench --csv --keep-cache album/ >> runs.csv"},
  };
  return s;
}

/// "SCALE,X,Y" -> three non-negative integers.
bool parse_cell(const std::string& s, int& scale, int& x, int& y) {
  const auto parts = gp::cli::split(s, ',');
  if (parts.size() != 3 || s.find(",,") != std::string::npos) return false;
  const auto a = gp::cli::parse_ll(parts[0]);
  const auto b = gp::cli::parse_ll(parts[1]);
  const auto c = gp::cli::parse_ll(parts[2]);
  if (!a || !b || !c || *a < 0 || *a > 30 || *b < 0 || *c < 0 || *b > 1000000 || *c > 1000000) {
    return false;
  }
  scale = static_cast<int>(*a);
  x = static_cast<int>(*b);
  y = static_cast<int>(*c);
  return true;
}

}  // namespace

int main(int argc, char** argv) {
  namespace cli = gp::cli;
  const cli::Spec spec = make_spec();
  cli::Args args;
  switch (cli::parse(spec, argc, argv, args)) {
    case cli::Parsed::Help:
      cli::print_help(std::cout, spec);
      return 0;
    case cli::Parsed::Version:
      thumtoo::print_version(std::cout);
      return 0;
    case cli::Parsed::Error:
      return cli::fail_usage(spec, args.errors());
    case cli::Parsed::Run:
      break;
  }

  const fs::path cache = args.has("cache") ? fs::path(args.get_string("cache", "")) : default_cache_root();
  const unsigned jobs = static_cast<unsigned>(args.get_int("jobs", 0, 0, 256));
  const int ladder_edge = args.get_int("ladder", 256, 0, 65535);
  const int tile_min_scale = args.get_int("min-scale", 0, 0, 30);
  const int tile_max_scale = args.get_int("max-scale", -1, -1, 30);
  const bool keep_cache = args.has("keep-cache");
  const bool do_probe = !args.has("no-probe");
  const gp::OutputMode mode = gp::resolve_output_mode(args);
  bool do_tiles = args.has("tiles") || args.has("min-scale") || args.has("max-scale");
  bool do_tile_cell = args.has("tile-cell") || args.has("cell");
  int cell_scale = 0, cell_x = 0, cell_y = 0;
  if (args.has("cell") && !parse_cell(args.get_string("cell", ""), cell_scale, cell_x, cell_y)) {
    args.add_error("invalid value '" + args.get_string("cell", "") +
                   "' for --cell (expected SCALE,X,Y: three non-negative integers, e.g. 0,1,2)");
  }
  if (do_tiles && tile_max_scale >= 0 && tile_min_scale > tile_max_scale) {
    args.add_error("--min-scale " + std::to_string(tile_min_scale) + " is coarser than --max-scale " +
                   std::to_string(tile_max_scale));
  }

  if (args.positionals().empty()) args.add_error("missing PATH argument");
  std::vector<fs::path> paths;
  for (const std::string& p : args.positionals()) {
    std::error_code ec;
    if (!fs::exists(p, ec)) {
      args.add_error("cannot read '" + p + "': no such file or directory");
    } else {
      paths.emplace_back(p);
    }
  }
  if (!args.errors().empty()) return cli::fail_usage(spec, args.errors());

  try {
    std::error_code ec;
    if (!keep_cache) fs::remove_all(cache, ec);
    fs::create_directories(cache);

    thumtoo::image_library_init();
    const auto t_all = std::chrono::steady_clock::now();

    // user.sqlite lives inside the bench cache too: a benchmark must not touch
    // the real data root (tags), and needs no writable $HOME.
    auto client = thumtoo::Client::open(cache, {}, jobs, cache / "data");
    std::vector<PhaseResult> phases;
    std::vector<std::string> uris;

    // Reset the statistics window, run `body`, wait for the workers, record.
    const auto phase = [&](std::string name, const std::function<void()>& body, bool record = true) {
      thumtoo::global_build_stats().reset();
      const auto t0 = std::chrono::steady_clock::now();
      body();
      client->drain();
      if (!record) return;
      PhaseResult r;
      r.name = std::move(name);
      r.wall_s = wall_s_now(t0);
      r.pretty = thumtoo::global_build_stats().summary_pretty();
      r.counters = snapshot_counters();
      phases.push_back(std::move(r));
    };

    if (do_probe || ladder_edge > 0 || do_tiles || do_tile_cell) {
      phase("probe", [&] {
        std::vector<std::string> sized;
        client->prepare_paths(paths, [&](std::string uri, thumtoo::SizeReply reply) {
          if (reply.size) sized.push_back(std::move(uri));
        });
        client->drain();
        uris = std::move(sized);
      }, do_probe);
    }

    if (ladder_edge > 0 && !uris.empty()) {
      phase("preview(ladder=" + std::to_string(ladder_edge) + ")", [&] {
        for (const auto& uri : uris) client->request_pixels(uri, ladder_edge, {});
      });
    }
    if (do_tile_cell && !uris.empty()) {
      phase("tile-cell(scale=" + std::to_string(cell_scale) + "," + std::to_string(cell_x) + "," +
                std::to_string(cell_y) + ")",
            [&] {
              for (const auto& uri : uris) client->request_tile(uri, cell_scale, cell_x, cell_y, {});
            });
    }
    if (do_tiles && !uris.empty()) {
      phase("tiles(min_scale=" + std::to_string(tile_min_scale) +
                " max_scale=" + std::to_string(tile_max_scale) + ")",
            [&] {
              for (const auto& uri : uris) {
                client->request_tile_pyramid(uri, tile_min_scale, tile_max_scale, {});
              }
            });
    }

    RunInfo run;
    run.cache = cache;
    run.keep_cache = keep_cache;
    run.paths = paths.size();
    run.uris = uris.size();
    run.workers = jobs ? jobs : std::max(1u, std::thread::hardware_concurrency());
    run.total_wall_s = wall_s_now(t_all);

    switch (mode) {
      case gp::OutputMode::Text:
        write_text(run, phases);
        break;
      case gp::OutputMode::Csv:
        write_csv(run, phases, !args.has("no-header"));
        break;
      case gp::OutputMode::Json:
        write_json(run, phases);
        break;
    }
    if (uris.empty()) {
      std::cerr << "thumtoo-bench: no readable image found in the given PATH(s)\n";
      return 1;
    }
  } catch (const std::exception& e) {
    std::cerr << "thumtoo-bench: error: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
