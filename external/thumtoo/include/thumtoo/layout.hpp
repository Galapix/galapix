// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <filesystem>

namespace thumtoo {

/// Store files always live at cache_root (index.sqlite, bulk.sqlite).
/// Classic dual-path trees are moved under cache_root/legacy/ on open.
/// THUMTOO_STORE_ROOT is ignored (nested store/ opt-out removed).

/// Destination for relocated pre-cutover files (layout migrate only).
[[nodiscard]] std::filesystem::path legacy_db_root(
    const std::filesystem::path& cache_root);

/// Store root: always cache_root (top-level layout).
[[nodiscard]] std::filesystem::path redesign_store_root(
    const std::filesystem::path& cache_root);

/// Move classic dual-path files into the root layout.
/// Safe to call repeatedly; skips renames when destinations exist.
void migrate_dual_path_to_store_root(const std::filesystem::path& cache_root);

/// Default root for user.sqlite (tags / collections): XDG_STATE_HOME/thumtoo
/// (not cache — user overlays must survive cache wipes). Same directory as
/// appearance state (`default_state_root`).
[[nodiscard]] std::filesystem::path default_data_root();

/// If data_root/user.sqlite is missing, move/copy from cache_root or legacy
/// XDG_DATA_HOME/thumtoo when present. Safe to call repeatedly.
void migrate_user_sqlite_to_data_root(const std::filesystem::path& cache_root,
                                      const std::filesystem::path& data_root);

}  // namespace thumtoo
