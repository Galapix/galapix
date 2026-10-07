// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "thumtoo/build_stats.hpp"

#include <iomanip>
#include <sstream>

namespace thumtoo {

BuildStats& global_build_stats() {
  static BuildStats s;
  return s;
}

void BuildStats::reset() {
  archive_extract_ns.store(0, std::memory_order_relaxed);
  image_load_ns.store(0, std::memory_order_relaxed);
  shrink_ns.store(0, std::memory_order_relaxed);
  jpeg_encode_ns.store(0, std::memory_order_relaxed);
  thumb_ns.store(0, std::memory_order_relaxed);
  jxl_encode_ns.store(0, std::memory_order_relaxed);
  levels_encoded.store(0, std::memory_order_relaxed);
  tiles_encoded.store(0, std::memory_order_relaxed);
  archive_bytes.store(0, std::memory_order_relaxed);
  exif_thumb_hits.store(0, std::memory_order_relaxed);
  probes_done.store(0, std::memory_order_relaxed);
  pixel_jobs.store(0, std::memory_order_relaxed);
  tile_jobs.store(0, std::memory_order_relaxed);
  wall_start = std::chrono::steady_clock::now();
}

namespace {

double ns_to_s(std::uint64_t ns) {
  return static_cast<double>(ns) / 1e9;
}

std::uint64_t wall_ns_since(std::chrono::steady_clock::time_point start) {
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          std::chrono::steady_clock::now() - start)
          .count());
}

void append_rate(std::ostringstream& out, const char* label, double count,
                 double wall_s) {
  if (wall_s <= 0 || count <= 0) return;
  out << "  " << label << "=" << std::fixed << std::setprecision(1)
      << (count / wall_s) << "/s";
}

}  // namespace

std::string BuildStats::summary_line() const {
  const auto extract = archive_extract_ns.load(std::memory_order_relaxed);
  const auto load = image_load_ns.load(std::memory_order_relaxed);
  const auto shrink = shrink_ns.load(std::memory_order_relaxed);
  const auto jpeg = jpeg_encode_ns.load(std::memory_order_relaxed);
  const auto thumb = thumb_ns.load(std::memory_order_relaxed);
  const auto jxl = jxl_encode_ns.load(std::memory_order_relaxed);
  const auto levels = levels_encoded.load(std::memory_order_relaxed);
  const auto tiles = tiles_encoded.load(std::memory_order_relaxed);
  const auto bytes = archive_bytes.load(std::memory_order_relaxed);
  const auto exif = exif_thumb_hits.load(std::memory_order_relaxed);
  const auto probes = probes_done.load(std::memory_order_relaxed);
  const auto cpu_sum = extract + load + shrink + jpeg + thumb + jxl;
  const auto wall_ns = wall_ns_since(wall_start);

  std::ostringstream out;
  out.setf(std::ios::fixed);
  out.precision(3);
  out << "timings: wall=" << ns_to_s(wall_ns) << "s"
      << " | cpu: extract=" << ns_to_s(extract) << "s"
      << " load=" << ns_to_s(load) << "s"
      << " shrink=" << ns_to_s(shrink) << "s"
      << " jpeg=" << ns_to_s(jpeg) << "s"
      << " thumb=" << ns_to_s(thumb) << "s"
      << " jxl=" << ns_to_s(jxl) << "s"
      << " (cpu-sum=" << ns_to_s(cpu_sum) << "s)"
      << " levels=" << levels << " tiles=" << tiles;
  if (probes) out << " probes=" << probes;
  if (exif) out << " exif_hits=" << exif;
  if (bytes > 0) {
    out.precision(1);
    out << " archive_MiB=" << (static_cast<double>(bytes) / (1024.0 * 1024.0));
  }
  if (cpu_sum > 0) {
    out.precision(0);
    out << " | cpu-share: extract=" << (100.0 * extract / cpu_sum) << "%"
        << " load=" << (100.0 * load / cpu_sum) << "%"
        << " shrink=" << (100.0 * shrink / cpu_sum) << "%"
        << " jpeg=" << (100.0 * jpeg / cpu_sum) << "%"
        << " thumb=" << (100.0 * thumb / cpu_sum) << "%"
        << " jxl=" << (100.0 * jxl / cpu_sum) << "%";
  }
  if (wall_ns > 0 && cpu_sum > 0) {
    out.precision(2);
    out << " | parallel~=" << (ns_to_s(cpu_sum) / ns_to_s(wall_ns)) << "x";
  }
  return out.str();
}

std::string BuildStats::summary_pretty() const {
  const auto extract = archive_extract_ns.load(std::memory_order_relaxed);
  const auto load = image_load_ns.load(std::memory_order_relaxed);
  const auto shrink = shrink_ns.load(std::memory_order_relaxed);
  const auto jpeg = jpeg_encode_ns.load(std::memory_order_relaxed);
  const auto thumb = thumb_ns.load(std::memory_order_relaxed);
  const auto jxl = jxl_encode_ns.load(std::memory_order_relaxed);
  const auto levels = levels_encoded.load(std::memory_order_relaxed);
  const auto tiles = tiles_encoded.load(std::memory_order_relaxed);
  const auto bytes = archive_bytes.load(std::memory_order_relaxed);
  const auto exif = exif_thumb_hits.load(std::memory_order_relaxed);
  const auto probes = probes_done.load(std::memory_order_relaxed);
  const auto pix_jobs = pixel_jobs.load(std::memory_order_relaxed);
  const auto til_jobs = tile_jobs.load(std::memory_order_relaxed);
  const auto cpu_sum = extract + load + shrink + jpeg + thumb + jxl;
  const auto wall_ns = wall_ns_since(wall_start);
  const double wall_s = ns_to_s(wall_ns);
  const double cpu_s = ns_to_s(cpu_sum);

  auto row = [](std::ostringstream& o, const char* name, std::uint64_t ns,
                std::uint64_t sum) {
    o << "  " << std::left << std::setw(10) << name << std::right
      << std::fixed << std::setprecision(3) << std::setw(10) << ns_to_s(ns)
      << " s";
    if (sum > 0 && ns > 0) {
      o << "  " << std::setw(5) << std::setprecision(1)
        << (100.0 * static_cast<double>(ns) / static_cast<double>(sum)) << "%";
    }
    o << "\n";
  };

  std::ostringstream out;
  out << "── thumtoo timings ──────────────────────────────────\n";
  out << std::fixed << std::setprecision(3);
  out << "  wall time      " << std::setw(10) << wall_s << " s\n";
  out << "  cpu-sum        " << std::setw(10) << cpu_s << " s";
  if (wall_s > 0 && cpu_s > 0) {
    out << "  (parallel~ " << std::setprecision(2) << (cpu_s / wall_s) << "x)";
  }
  out << "\n";
  out << "  note: cpu-* scopes sum across worker threads; can exceed wall.\n";
  out << "\n";
  out << "  category        seconds   share\n";
  out << "  ──────────    ────────  ─────\n";
  row(out, "extract", extract, cpu_sum);
  row(out, "load", load, cpu_sum);
  row(out, "shrink", shrink, cpu_sum);
  row(out, "jpeg", jpeg, cpu_sum);
  row(out, "thumb", thumb, cpu_sum);
  row(out, "jxl", jxl, cpu_sum);
  out << "\n";
  out << "  outputs\n";
  out << "    probes=" << probes << "  pixel_jobs=" << pix_jobs
      << "  tile_jobs=" << til_jobs << "\n";
  out << "    levels=" << levels << "  tiles=" << tiles
      << "  exif_hits=" << exif << "\n";
  if (bytes > 0) {
    out << std::setprecision(2);
    out << "    archive_extracted="
        << (static_cast<double>(bytes) / (1024.0 * 1024.0)) << " MiB\n";
  }
  out << "\n";
  out << "  rates (wall)";
  append_rate(out, "probes", static_cast<double>(probes), wall_s);
  append_rate(out, "levels", static_cast<double>(levels), wall_s);
  append_rate(out, "tiles", static_cast<double>(tiles), wall_s);
  out << "\n";
  if (levels > 0 && (thumb + jxl) > 0) {
    out << std::setprecision(1);
    out << "  per-level (thumb+jxl cpu / levels)="
        << (1000.0 * ns_to_s(thumb + jxl) / static_cast<double>(levels))
        << " ms\n";
  }
  if (tiles > 0 && (load + shrink + jpeg) > 0) {
    out << std::setprecision(1);
    out << "  per-tile (load+shrink+jpeg cpu / tiles)="
        << (1000.0 * ns_to_s(load + shrink + jpeg) / static_cast<double>(tiles))
        << " ms\n";
  }
  out << "────────────────────────────────────────────────────\n";
  return out.str();
}

}  // namespace thumtoo
