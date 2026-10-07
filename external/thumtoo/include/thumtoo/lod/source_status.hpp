// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "thumtoo/lod/source_records.hpp"
#include "thumtoo/lod/tile_session.hpp"

#include <thumtoo/pdf.hpp>

#include <optional>
#include <string>
#include <vector>

namespace thumtoo::lod {

/**
 * Everything the Status panel shows about one source, gathered in one place:
 * the source record (facts + decisions), the focused view's tile session and
 * the shared loader, and thumtoo's render/decode accounting.
 */
struct SourceStatus {
  SourceRecord record;
  std::optional<TileSession::DebugSnapshot> tiles;  ///< focused view, if any
  std::string tile_error;                            ///< first visible failure
  std::optional<thumtoo::PdfDocumentRenderStats> render;
  /// Shown in the "Decisions (<host>)" section title.
  std::string host = "host";
};

enum class StatusTone { Normal, Good, Warn, Bad };

struct StatusRow {
  std::string label;
  std::string value;
  StatusTone tone = StatusTone::Normal;
};

struct StatusSection {
  std::string title;
  std::vector<StatusRow> rows;
};

/// Presentation-neutral rows for the panel (HTML) and the copied report.
[[nodiscard]] std::vector<StatusSection> build_status_sections(SourceStatus const& s);

/// Plain-text report ("Title\n  label: value\n…").
[[nodiscard]] std::string format_status_text(std::vector<StatusSection> const& sections);

}  // namespace thumtoo::lod

