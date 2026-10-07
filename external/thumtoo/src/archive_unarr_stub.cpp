// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "thumtoo/archive.hpp"

namespace thumtoo {

bool unarr_backend_available() { return false; }

bool archive_prefers_unarr(const std::filesystem::path&) { return false; }

bool archive_is_rar5(const std::filesystem::path&) { return false; }

std::optional<std::vector<ArchiveMember>> read_archive_toc_unarr(
    const std::filesystem::path&) {
  return std::nullopt;
}

std::unordered_map<std::string, std::vector<std::uint8_t>>
extract_archive_members_unarr(const std::filesystem::path&,
                              const std::vector<std::string>&) {
  return {};
}

std::size_t visit_archive_members_unarr(const std::filesystem::path&,
                                        const std::vector<std::string>&,
                                        const ArchiveMemberVisitor&) {
  return 0;
}

}  // namespace thumtoo
