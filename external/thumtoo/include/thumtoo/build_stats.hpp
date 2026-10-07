// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <string>

namespace thumtoo {

/**
 * Timings for prepare / tile / preview generation.
 *
 * Per-category counters accumulate *thread wall time* (steady_clock scopes).
 * With a multi-worker pool those scopes overlap, so category sums can exceed
 * real elapsed time — treat them as CPU-ish cost, not wall duration.
 * summary_line() / summary_pretty() also report true wall time since reset().
 */
struct BuildStats {
  std::atomic<std::uint64_t> archive_extract_ns{0};
  std::atomic<std::uint64_t> image_load_ns{0};
  std::atomic<std::uint64_t> shrink_ns{0};
  std::atomic<std::uint64_t> jpeg_encode_ns{0};
  /// Single-edge preview (vips_thumbnail / EXIF + JXL) — not the tile pyramid.
  std::atomic<std::uint64_t> thumb_ns{0};
  std::atomic<std::uint64_t> jxl_encode_ns{0};
  std::atomic<std::uint64_t> levels_encoded{0};
  std::atomic<std::uint64_t> tiles_encoded{0};
  std::atomic<std::uint64_t> archive_bytes{0};
  /// EXIF embedded JPEG thumb used for preview (skip full decode).
  std::atomic<std::uint64_t> exif_thumb_hits{0};
  std::atomic<std::uint64_t> probes_done{0};
  std::atomic<std::uint64_t> pixel_jobs{0};
  std::atomic<std::uint64_t> tile_jobs{0};

  void reset();

  /// One-line summary (backward compatible, for scripts).
  [[nodiscard]] std::string summary_line() const;

  /// Multi-line human-readable report (tables + rates).
  [[nodiscard]] std::string summary_pretty() const;

  /// Wall time of the last reset() (for elapsed calculation).
  std::chrono::steady_clock::time_point wall_start{};
};

BuildStats& global_build_stats();

/** Adds elapsed steady_clock time to \a counter (sums across threads). */
class ScopedNsAccumulator {
 public:
  explicit ScopedNsAccumulator(std::atomic<std::uint64_t>& counter)
      : counter_(counter), t0_(std::chrono::steady_clock::now()) {}
  ScopedNsAccumulator(const ScopedNsAccumulator&) = delete;
  ScopedNsAccumulator& operator=(const ScopedNsAccumulator&) = delete;
  ~ScopedNsAccumulator() {
    const auto dt = std::chrono::steady_clock::now() - t0_;
    const auto ns =
        std::chrono::duration_cast<std::chrono::nanoseconds>(dt).count();
    if (ns > 0) {
      counter_.fetch_add(static_cast<std::uint64_t>(ns),
                         std::memory_order_relaxed);
    }
  }

 private:
  std::atomic<std::uint64_t>& counter_;
  std::chrono::steady_clock::time_point t0_;
};

}  // namespace thumtoo
