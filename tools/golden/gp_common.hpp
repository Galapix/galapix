// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Shared helpers for the golden-path tools (tools/golden/gp_*.cpp).
// Header-only and free of thumtoo library dependencies on purpose: golden
// tools measure "the obvious correct thing" without Client/Store linkage.

#pragma once

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <functional>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

#include <fcntl.h>
#include <unistd.h>

namespace gp {

using clock_type = std::chrono::steady_clock;

inline double ms_since(clock_type::time_point t0) {
  return std::chrono::duration<double, std::milli>(clock_type::now() - t0)
      .count();
}

/// Wall-clock summary of `samples` timed runs (after one untimed warmup).
struct Timing {
  double median = 0;
  double min = 0;
  double max = 0;
  int samples = 0;
};

/// One untimed warmup call, then `repeats` timed calls (at least one).
inline Timing time_median(int repeats, const std::function<void()>& fn) {
  repeats = std::max(1, repeats);
  std::vector<double> times;
  times.reserve(static_cast<std::size_t>(repeats));
  fn();
  for (int i = 0; i < repeats; ++i) {
    const auto t0 = clock_type::now();
    fn();
    times.push_back(ms_since(t0));
  }
  std::sort(times.begin(), times.end());
  return Timing{times[times.size() / 2], times.front(), times.back(),
                static_cast<int>(times.size())};
}

/// Time several competing functions fairly: one untimed warmup each, then
/// `repeats` rounds in which every function runs once. The starting function
/// rotates each round, so warm-up drift (page cache, allocator, CPU clocks)
/// is spread over all of them instead of favoring whichever runs last.
/// Result i belongs to fns[i].
inline std::vector<Timing> time_interleaved(int repeats,
                                            const std::vector<std::function<void()>>& fns) {
  repeats = std::max(1, repeats);
  const std::size_t n = fns.size();
  std::vector<std::vector<double>> times(n);
  for (const auto& fn : fns) fn();
  for (int round = 0; round < repeats; ++round) {
    for (std::size_t k = 0; k < n; ++k) {
      const std::size_t i = (k + static_cast<std::size_t>(round)) % n;
      const auto t0 = clock_type::now();
      fns[i]();
      times[i].push_back(ms_since(t0));
    }
  }
  std::vector<Timing> out;
  for (auto& t : times) {
    std::sort(t.begin(), t.end());
    out.push_back(Timing{t[t.size() / 2], t.front(), t.back(), static_cast<int>(t.size())});
  }
  return out;
}

/// Silences file descriptor 2 while alive.
///
/// libunarr and libvips write diagnostics straight to stderr ("Skipping
/// directory entry", format warnings) on every call, and a timing loop makes
/// thousands of calls. Wrap measurement code in this; our own messages are
/// printed outside it. Failures are still reported, through the tools' own
/// status/reason text. No-op if /dev/null or dup() is unavailable.
class StderrSilencer {
 public:
  StderrSilencer() {
    std::fflush(stderr);
    saved_ = ::dup(STDERR_FILENO);
    const int null_fd = ::open("/dev/null", O_WRONLY);
    if (saved_ >= 0 && null_fd >= 0) ::dup2(null_fd, STDERR_FILENO);
    if (null_fd >= 0) ::close(null_fd);
  }
  ~StderrSilencer() {
    if (saved_ >= 0) {
      std::fflush(stderr);
      ::dup2(saved_, STDERR_FILENO);
      ::close(saved_);
    }
  }
  StderrSilencer(const StderrSilencer&) = delete;
  StderrSilencer& operator=(const StderrSilencer&) = delete;

 private:
  int saved_ = -1;
};

/// Write `s` as a JSON string literal (quotes included).
inline void json_string(std::ostream& os, std::string_view s) {
  os << '"';
  for (const char ch : s) {
    const auto c = static_cast<unsigned char>(ch);
    switch (c) {
      case '"': os << "\\\""; break;
      case '\\': os << "\\\\"; break;
      case '\n': os << "\\n"; break;
      case '\r': os << "\\r"; break;
      case '\t': os << "\\t"; break;
      default:
        if (c < 0x20) {
          char buf[8];
          std::snprintf(buf, sizeof(buf), "\\u%04x", c);
          os << buf;
        } else {
          os << ch;
        }
    }
  }
  os << '"';
}

}  // namespace gp
