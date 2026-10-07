// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

/** Set when ContentAppearance has grade_invert. */
#define THUMTOO_APPEARANCE_GRADE_INVERT 1

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace thumtoo {

/// Non-destructive content appearance (no placement / Workspace pose).
/// Identity = all flags false, quarter turns 0, no crop, no grade.
struct ContentAppearance {
  bool content_h_flip = false;
  bool content_v_flip = false;
  int content_quarter_turns = 0;
  bool has_crop = false;
  int crop_x = 0;
  int crop_y = 0;
  int crop_w = 0;
  int crop_h = 0;
  int crop_source_w = 0;
  int crop_source_h = 0;
  double crop_rotation = 0.0;
  /// Optional colour grade; nullopt = identity / not stored.
  std::optional<int> grade_brightness;
  std::optional<int> grade_contrast;
  std::optional<int> grade_saturation;
  std::optional<int> grade_hue;
  std::optional<int> grade_gamma;
  std::optional<int> grade_invert;  ///< 0/1 photographic negative

  [[nodiscard]] bool is_identity() const;
};

/// ${XDG_STATE_HOME:-$HOME/.local/state}/thumtoo — never under the source tree.
[[nodiscard]] std::filesystem::path default_state_root();

/// Normalize to "sha256:" + lowercase hex, or empty if invalid.
[[nodiscard]] std::string normalize_content_id(std::string_view content_id_or_hex);

/// Durable content-appearance store (SQLite under the state root).
/// Not the pixel cache: lives under XDG_STATE_HOME, not XDG_CACHE_HOME.
class AppearanceStore {
 public:
  AppearanceStore() = default;
  AppearanceStore(const AppearanceStore&) = delete;
  AppearanceStore& operator=(const AppearanceStore&) = delete;
  AppearanceStore(AppearanceStore&&) noexcept;
  AppearanceStore& operator=(AppearanceStore&&) noexcept;
  ~AppearanceStore();

  /// Open or create appearance.sqlite3 under @p state_root.
  /// On failure returns an invalid store (valid() == false); does not throw.
  [[nodiscard]] static AppearanceStore open(
      const std::filesystem::path& state_root = default_state_root());

  [[nodiscard]] bool valid() const { return db_ != nullptr; }
  [[nodiscard]] const std::filesystem::path& db_path() const { return db_path_; }

  [[nodiscard]] std::optional<ContentAppearance> get(
      std::string_view content_id) const;

  /// Insert/update. Identity appearance removes the row.
  void put(std::string_view content_id, const ContentAppearance& app);

  void remove(std::string_view content_id);

 private:
  explicit AppearanceStore(void* db, std::filesystem::path db_path);

  void* db_ = nullptr;  // sqlite3*
  std::filesystem::path db_path_;
};

}  // namespace thumtoo
