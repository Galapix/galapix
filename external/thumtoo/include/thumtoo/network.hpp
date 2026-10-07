// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace thumtoo {

/// True if this build was linked with libcurl (THUMTOO_HAVE_CURL).
[[nodiscard]] bool http_fetch_available();

/**
 * HTTP(S) GET into memory. Requires THUMTOO_HAVE_CURL.
 * Rejects responses larger than max_bytes (and aborts the transfer).
 * Follows redirects. Returns nullopt on error or when curl is unavailable.
 */
[[nodiscard]] std::optional<std::vector<std::uint8_t>> http_get_bytes(
    std::string_view url, std::uint64_t max_bytes);

}  // namespace thumtoo
