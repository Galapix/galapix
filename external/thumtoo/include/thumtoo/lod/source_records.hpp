// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <thumtoo/pdf.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace thumtoo::lod {

/**
 * What the host knows and decided about one source (session path), as plain
 * data. Systems write facts and decisions here; views and the Status panel
 * read them. No signals: readers poll or re-plan on the path's view hook.
 *
 * Every heuristic or policy choice that affects how a source is shown goes
 * into `decisions` with the value and the reason, so it can be displayed.
 * See docs/SOURCE_RECORDS.md.
 */

enum class SourceKind {
  Image,     ///< raster file (pyramid scales >= 0)
  PdfPage,   ///< MuPDF page: PDF / Markdown / plain text //page:N
  DjvuPage,
  EpubPage,
};

[[nodiscard]] char const* source_kind_name(SourceKind kind);

enum class ProfileState {
  NotApplicable,  ///< no page profile for this kind of source
  Unknown,        ///< not requested yet
  Pending,        ///< requested; thumtoo is building the display list
  Known,
  Failed,
};

[[nodiscard]] char const* profile_state_name(ProfileState state);

struct Decision {
  std::string what;   ///< stable key, e.g. "zoom floor"
  std::string value;  ///< e.g. "scale -1 (288 dpi)"
  std::string why;    ///< one line
  std::int64_t at_ms = 0;
};

struct SourceRecord {
  std::string path;
  SourceKind kind = SourceKind::Image;

  ProfileState profile_state = ProfileState::Unknown;
  std::optional<thumtoo::PdfPageProfile> profile;
  std::string profile_error;
  std::int64_t profile_requested_ms = 0;
  std::int64_t profile_ms = 0;  ///< wall time thumtoo took (request → answer)
  std::uint64_t profile_request = 0;  ///< answers for older requests are dropped

  /// Underlying document file and page for PdfPage (render stats lookup).
  std::string document_file;
  int page = 0;

  std::vector<Decision> decisions;  ///< one per `what`, latest value

  [[nodiscard]] Decision const* decision(std::string const& what) const;
};

/// Finest tile scale a view may plan for a source, and why.
struct ZoomFloor {
  int min_scale = 0;
  bool provisional = false;  ///< will change once the profile arrives
  std::string reason;
};

/**
 * Policy: how deep a source may be zoomed with live tiles.
 *
 * - Images: scale 0 (full resolution).
 * - PDF pages: Raster pages stop at the profile's finest useful scale;
 *   Vector / Mixed / Empty pages go to @p document_floor. Until the profile is
 *   known the floor is the layout scale (0) — provisional, never a guess.
 * - EPUB pages: the same rules as PDF (laid-out pages run on the same
 *   MuPDF runtime). DjVu pages: 0 — layout is their native pixel grid.
 */
[[nodiscard]] ZoomFloor decide_zoom_floor(SourceRecord const& record,
                                          int document_floor);

/**
 * Process-wide table keyed by session path. GUI thread only.
 */
class SourceRecords {
public:
  static SourceRecords& instance();

  /// Existing record or a new one (kind from @p kind on creation).
  SourceRecord& ensure(std::string const& path, SourceKind kind);
  [[nodiscard]] SourceRecord const* find(std::string const& path) const;
  SourceRecord* find_mutable(std::string const& path);

  /// Replace the decision @p what (no-op when value and reason are unchanged).
  /// @return true when it changed.
  bool decide(std::string const& path, std::string const& what,
              std::string const& value, std::string const& why,
              std::int64_t now_ms);

  /// The decision @p what no longer applies (e.g. the view left Gallery).
  /// @return true when one was removed.
  bool retract(std::string const& path, std::string const& what);

  /// File changed / session replaced: forget what was learned about @p path.
  void forget(std::string const& path);
  void clear();

  /// Bumped on every change (cheap "did anything change" for pollers).
  [[nodiscard]] std::uint64_t generation() const { return m_generation; }
  void touch() { ++m_generation; }

  [[nodiscard]] std::size_t size() const { return m_records.size(); }

private:
  SourceRecords() = default;
  std::unordered_map<std::string, SourceRecord> m_records;
  std::uint64_t m_generation = 0;
};

}  // namespace thumtoo::lod

