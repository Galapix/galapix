// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdarg>
#include <cstdio>
#include <cstdlib>

namespace thumtoo {

/// Non-empty env is on; 0 / false / no (first letter) is off.
[[nodiscard]] inline bool env_flag_on(const char* name) {
  const char* e = std::getenv(name);
  if (!e || !e[0] || e[0] == '0') return false;
  if (e[0] == 'f' || e[0] == 'F' || e[0] == 'n' || e[0] == 'N') return false;
  return true;
}

/// THUMTOO_DEBUG=1 or BILTOO_THUMTOO_DEBUG=1 — general Client task traces.
[[nodiscard]] inline bool debug_enabled() {
  return env_flag_on("THUMTOO_DEBUG") || env_flag_on("BILTOO_THUMTOO_DEBUG");
}

/// Archive open / TOC / extract / visit. Also on when THUMTOO_DEBUG is set.
/// THUMTOO_DEBUG_ARCHIVE=1 alone is enough for archive-only noise.
[[nodiscard]] inline bool archive_debug_enabled() {
  return env_flag_on("THUMTOO_DEBUG_ARCHIVE") || debug_enabled();
}

inline void thumtoo_log(const char* prefix, const char* fmt, ...) {
  char buf[2048];
  va_list ap;
  va_start(ap, fmt);
  std::vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  std::fputs(prefix, stderr);
  std::fputs(buf, stderr);
  std::fputc('\n', stderr);
  std::fflush(stderr);
}

#define THUMTOO_DBG(...)                                                       \
  do {                                                                         \
    if (::thumtoo::debug_enabled())                                            \
      ::thumtoo::thumtoo_log("thumtoo: ", __VA_ARGS__);                        \
  } while (0)

#define THUMTOO_ARCHIVE_DBG(...)                                               \
  do {                                                                         \
    if (::thumtoo::archive_debug_enabled())                                    \
      ::thumtoo::thumtoo_log("thumtoo-archive: ", __VA_ARGS__);                \
  } while (0)

}  // namespace thumtoo
