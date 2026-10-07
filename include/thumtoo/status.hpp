// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string_view>

namespace thumtoo {

/// content.status (DESIGN — Status enum).
enum class ContentStatus : int {
  Pending = 0,      ///< Locator known; not yet successfully probed
  Ready = 1,        ///< Size (and requested levels) valid
  Failed = 2,       ///< Probe/decode failed; may retry later
  Unsupported = 3,  ///< Codec/container not handled; stop retrying
  Incomplete = 4,   ///< Partial success (e.g. size known, levels missing)
};

inline constexpr std::string_view to_string(ContentStatus s) {
  switch (s) {
    case ContentStatus::Pending:
      return "pending";
    case ContentStatus::Ready:
      return "ready";
    case ContentStatus::Failed:
      return "failed";
    case ContentStatus::Unsupported:
      return "unsupported";
    case ContentStatus::Incomplete:
      return "incomplete";
  }
  return "unknown";
}

}  // namespace thumtoo
