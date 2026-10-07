// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "thumtoo/lod/source_records.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace thumtoo::lod {

namespace {

std::string scale_text(int scale)
{
  char buf[64];
  std::snprintf(buf, sizeof buf, "scale %d (%.0f dpi)", scale,
                thumtoo::kPdfLayoutDpi * std::ldexp(1.0, -scale));
  return buf;
}

}  // namespace

char const* source_kind_name(SourceKind kind)
{
  switch (kind) {
  case SourceKind::Image:
    return "image";
  case SourceKind::PdfPage:
    return "PDF page";
  case SourceKind::DjvuPage:
    return "DjVu page";
  case SourceKind::EpubPage:
    return "EPUB page";
  }
  return "?";
}

char const* profile_state_name(ProfileState state)
{
  switch (state) {
  case ProfileState::NotApplicable:
    return "n/a";
  case ProfileState::Unknown:
    return "not requested";
  case ProfileState::Pending:
    return "pending";
  case ProfileState::Known:
    return "known";
  case ProfileState::Failed:
    return "failed";
  }
  return "?";
}

Decision const* SourceRecord::decision(std::string const& what) const
{
  for (Decision const& d : decisions) {
    if (d.what == what) {
      return &d;
    }
  }
  return nullptr;
}

ZoomFloor decide_zoom_floor(SourceRecord const& record, int document_floor)
{
  ZoomFloor f;
  switch (record.kind) {
  case SourceKind::Image:
    f.min_scale = 0;
    f.reason = "raster image: scale 0 is the file's full resolution";
    return f;
  case SourceKind::PdfPage:
  case SourceKind::DjvuPage:
  case SourceKind::EpubPage:
    break;
  }
  switch (record.profile_state) {
  case ProfileState::NotApplicable:
  case ProfileState::Unknown:
  case ProfileState::Pending:
    f.min_scale = 0;
    f.provisional = true;
    f.reason = "page profile " + std::string(profile_state_name(record.profile_state))
               + ": layout scale until thumtoo reports what the page draws";
    return f;
  case ProfileState::Failed:
    f.min_scale = 0;
    f.reason = "page profile failed (" + record.profile_error + "): layout scale only";
    return f;
  case ProfileState::Known:
    break;
  }
  thumtoo::PdfPageProfile const& p = *record.profile;
  if (record.kind == SourceKind::DjvuPage) {
    // Layout is the page's native pixel grid: scale 0 shows every pixel.
    f.min_scale = std::max(document_floor, p.finest_useful_scale.value_or(0));
    char buf[160];
    std::snprintf(buf, sizeof buf,
                  "DjVu page (raster, %.0f dpi): scale %d is the page's native pixels",
                  p.native_dpi, f.min_scale);
    f.reason = buf;
    return f;
  }
  if (p.kind == thumtoo::PageContentKind::Raster && p.finest_useful_scale) {
    f.min_scale = std::max(document_floor, *p.finest_useful_scale);
    char buf[160];
    std::snprintf(buf, sizeof buf, "raster page, sharpest image %.0f dpi: ", p.native_dpi);
    f.reason = buf + scale_text(f.min_scale) + " shows every image pixel";
    if (*p.finest_useful_scale < document_floor) {
      f.reason += " (capped by the document floor)";
    }
    return f;
  }
  f.min_scale = document_floor;
  f.reason = std::string(thumtoo::page_content_kind_name(p.kind))
             + " page: no resolution cap; document floor " + scale_text(document_floor);
  return f;
}

SourceRecords& SourceRecords::instance()
{
  static SourceRecords records;
  return records;
}

SourceRecord& SourceRecords::ensure(std::string const& path, SourceKind kind)
{
  auto it = m_records.find(path);
  if (it != m_records.end()) {
    return it->second;
  }
  SourceRecord r;
  r.path = path;
  r.kind = kind;
  r.profile_state = kind == SourceKind::Image ? ProfileState::NotApplicable
                                              : ProfileState::Unknown;
  ++m_generation;
  return m_records.emplace(path, std::move(r)).first->second;
}

SourceRecord const* SourceRecords::find(std::string const& path) const
{
  auto it = m_records.find(path);
  return it == m_records.end() ? nullptr : &it->second;
}

SourceRecord* SourceRecords::find_mutable(std::string const& path)
{
  auto it = m_records.find(path);
  return it == m_records.end() ? nullptr : &it->second;
}

bool SourceRecords::decide(std::string const& path, std::string const& what,
                           std::string const& value, std::string const& why,
                           std::int64_t now_ms)
{
  SourceRecord* r = find_mutable(path);
  if (!r) {
    return false;
  }
  for (Decision& d : r->decisions) {
    if (d.what == what) {
      if (d.value == value && d.why == why) {
        return false;
      }
      d.value = value;
      d.why = why;
      d.at_ms = now_ms;
      ++m_generation;
      return true;
    }
  }
  r->decisions.push_back({what, value, why, now_ms});
  ++m_generation;
  return true;
}

bool SourceRecords::retract(std::string const& path, std::string const& what)
{
  SourceRecord* r = find_mutable(path);
  if (!r) {
    return false;
  }
  auto const before = r->decisions.size();
  r->decisions.erase(std::remove_if(r->decisions.begin(), r->decisions.end(),
                                    [&](Decision const& d) { return d.what == what; }),
                     r->decisions.end());
  if (r->decisions.size() == before) {
    return false;
  }
  ++m_generation;
  return true;
}

void SourceRecords::forget(std::string const& path)
{
  if (m_records.erase(path) > 0) {
    ++m_generation;
  }
}

void SourceRecords::clear()
{
  m_records.clear();
  ++m_generation;
}

}  // namespace thumtoo::lod
