// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "thumtoo/lod/source_status.hpp"

#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdio>

namespace thumtoo::lod {

namespace {

std::string fmt(char const* f, ...)
{
  char buf[512];
  va_list ap;
  va_start(ap, f);
  std::vsnprintf(buf, sizeof buf, f, ap);
  va_end(ap);
  return buf;
}

bool is_document(SourceKind k)
{
  return k != SourceKind::Image;
}

/// @p base_dpi: dpi of scale 0 (144 for PDF layout, the page dpi for DjVu,
/// 0 for images).
std::string scale_text(int scale, double base_dpi)
{
  if (base_dpi <= 0.0) {
    return fmt("%d (1/%d)", scale, 1 << std::max(0, scale));
  }
  return fmt("%d (%.0f dpi)", scale, base_dpi * std::ldexp(1.0, -scale));
}

char const* phase_name(TileSession::Phase p)
{
  return TileSession::phase_name(p);
}

StatusTone phase_tone(TileSession::Phase p)
{
  switch (p) {
  case TileSession::Phase::Complete:
    return StatusTone::Good;
  case TileSession::Phase::Loading:
  case TileSession::Phase::Idle:
    return StatusTone::Normal;
  case TileSession::Phase::Degraded:
    return StatusTone::Warn;
  case TileSession::Phase::Error:
    return StatusTone::Bad;
  }
  return StatusTone::Normal;
}

std::string image_text(thumtoo::PdfPageImage const& im)
{
  char const* cs = im.stencil         ? "stencil"
                   : im.components == 1 ? "gray"
                   : im.components == 3 ? "RGB"
                   : im.components == 4 ? "CMYK"
                                        : "?";
  return fmt("%d×%d %s %d-bit · %.0f dpi · %.0f%% of page", im.width, im.height, cs, im.bpc,
             im.dpi, im.page_fraction * 100.0);
}

void add_decode_rows(std::vector<StatusRow>& rows, thumtoo::PdfDecodeStats const& d,
                     std::int64_t cells, bool djvu)
{
  if (djvu) {
    rows.push_back({"page decodes",
                    fmt("%lld for %lld cells — ideal is one per page (decoded pages are "
                        "kept)",
                        static_cast<long long>(d.decodes), static_cast<long long>(cells))});
    if (d.decodes > 0) {
      rows.push_back({"decoded", fmt("%.1f MP in %.1f ms",
                                     static_cast<double>(d.decoded_pixels) / 1e6,
                                     d.decode_ms)});
    }
    return;
  }
  if (d.decodes == 0 && d.shared_waits == 0) {
    rows.push_back({"image decodes", "none"});
    return;
  }
  StatusTone tone = StatusTone::Normal;
  if (d.subarea_decodes > 0) {
    tone = StatusTone::Warn;
  } else if (d.decodes > 0) {
    tone = StatusTone::Good;
  }
  rows.push_back({"image decodes",
                  fmt("%lld (whole image %lld, per-cell subarea %lld) for %lld cells — "
                      "ideal is one per image and zoom level",
                      static_cast<long long>(d.decodes), static_cast<long long>(d.full_decodes),
                      static_cast<long long>(d.subarea_decodes),
                      static_cast<long long>(cells)),
                  tone});
  if (d.decodes > 0) {
    rows.push_back({"decoded", fmt("%.1f MP in %.1f ms (largest %.1f MP)",
                                   static_cast<double>(d.decoded_pixels) / 1e6, d.decode_ms,
                                   static_cast<double>(d.largest_decode_pixels) / 1e6)});
  }
  if (d.shared_waits > 0) {
    rows.push_back({"shared decodes",
                    fmt("%lld cells waited for another thread's decode",
                        static_cast<long long>(d.shared_waits))});
  }
  if (!d.last_subarea_reason.empty()) {
    rows.push_back({"why per-cell", d.last_subarea_reason, StatusTone::Warn});
  }
}

}  // namespace

std::vector<StatusSection> build_status_sections(SourceStatus const& s)
{
  std::vector<StatusSection> out;
  SourceRecord const& r = s.record;
  bool const doc = is_document(r.kind);
  double base_dpi = 0.0;
  bool const mupdf = r.kind == SourceKind::PdfPage || r.kind == SourceKind::EpubPage;
  if (mupdf) {
    base_dpi = thumtoo::kPdfLayoutDpi;
  } else if (r.kind == SourceKind::DjvuPage && r.profile && r.profile->native_dpi > 0) {
    base_dpi = r.profile->native_dpi;
  }

  {
    StatusSection sec{"Source", {}};
    sec.rows.push_back({"path", r.path});
    sec.rows.push_back({"kind", source_kind_name(r.kind)});
    if (!r.document_file.empty()) {
      sec.rows.push_back({"document", fmt("%s, page %d", r.document_file.c_str(), r.page)});
    }
    if (s.tiles && s.tiles->content_w > 0) {
      std::string at;
      if (mupdf) {
        at = " at 144 dpi";
      } else if (base_dpi > 0) {
        at = fmt(" (native, %.0f dpi)", base_dpi);
      }
      sec.rows.push_back({doc ? "layout size" : "size",
                          fmt("%d×%d px%s", s.tiles->content_w, s.tiles->content_h,
                              at.c_str())});
    }
    out.push_back(std::move(sec));
  }

  if (doc) {
    StatusSection sec{"Page analysis (thumtoo)", {}};
    StatusTone tone = StatusTone::Normal;
    if (r.profile_state == ProfileState::Known) {
      tone = StatusTone::Good;
    } else if (r.profile_state == ProfileState::Failed) {
      tone = StatusTone::Bad;
    }
    std::string state = profile_state_name(r.profile_state);
    if (r.profile_state == ProfileState::Known || r.profile_state == ProfileState::Failed) {
      state += fmt(" (%lld ms)", static_cast<long long>(r.profile_ms));
    }
    sec.rows.push_back({"profile", state, tone});
    if (r.profile_state == ProfileState::Failed) {
      sec.rows.push_back({"error", r.profile_error, StatusTone::Bad});
    }
    if (r.profile) {
      thumtoo::PdfPageProfile const& p = *r.profile;
      sec.rows.push_back({"content", thumtoo::page_content_kind_name(p.kind)});
      sec.rows.push_back({"verdict", p.summary});
      sec.rows.push_back({"page", fmt("%.0f×%.0f pt (%.1f×%.1f in)", p.width_pt, p.height_pt,
                                      p.width_pt / 72.0, p.height_pt / 72.0)});
      for (std::size_t i = 0; i < p.images.size(); ++i) {
        sec.rows.push_back({fmt("image %zu", i + 1), image_text(p.images[i])});
      }
      if (p.image_draws > static_cast<int>(p.images.size())) {
        sec.rows.push_back({"more images",
                            fmt("%d further draws not listed",
                                p.image_draws - static_cast<int>(p.images.size()))});
      }
      if (r.kind == SourceKind::DjvuPage) {
        sec.rows.push_back({"hidden text", p.invisible_glyphs > 0
                                               ? fmt("%d glyphs", p.invisible_glyphs)
                                               : std::string("none")});
      } else {
        sec.rows.push_back({"vector", fmt("%d paths, %d shadings (%.0f%% of page)",
                                          p.vector_paths, p.shadings,
                                          p.vector_coverage * 100.0)});
        sec.rows.push_back({"glyphs", fmt("%d visible, %d clip, %d invisible (OCR)",
                                          p.visible_glyphs, p.clip_glyphs,
                                          p.invisible_glyphs)});
      }
      if (p.background_fill_ignored) {
        sec.rows.push_back({"background", "page-size fill treated as paper, not detail"});
      }
    }
    out.push_back(std::move(sec));
  }

  if (!r.decisions.empty()) {
    StatusSection sec{"Decisions (" + s.host + ")", {}};
    for (Decision const& d : r.decisions) {
      sec.rows.push_back({d.what, d.value + " — " + d.why});
    }
    out.push_back(std::move(sec));
  }

  if (s.tiles) {
    TileSession::DebugSnapshot const& t = *s.tiles;
    StatusSection sec{"Tiles (this view)", {}};
    sec.rows.push_back({"state", phase_name(t.phase), phase_tone(t.phase)});
    sec.rows.push_back({"target scale", scale_text(t.target_scale, base_dpi)});
    sec.rows.push_back({"scale range", fmt("%s … %s", scale_text(t.min_scale, base_dpi).c_str(),
                                           scale_text(t.max_scale, base_dpi).c_str())});
    TileSession::Coverage const& c = t.cov;
    sec.rows.push_back({"visible cells",
                        fmt("%d: %d ready, %d queued, %d missing, %d retrying, %d failed, "
                            "%d unavailable",
                            c.visible, c.ready, c.queued, c.missing, c.retrying, c.failed,
                            c.unavailable),
                        c.failed + c.unavailable > 0 ? StatusTone::Warn : StatusTone::Normal});
    if (c.holes > 0) {
      sec.rows.push_back({"holes", fmt("%d visible cells with no pixels at any scale", c.holes),
                          StatusTone::Bad});
    }
    sec.rows.push_back({"drawn", fmt("%d exact, %d from coarser levels, %d underlay, %d empty",
                                     t.plan_exact, t.plan_parent, t.plan_underlay,
                                     t.plan_empty)});
    sec.rows.push_back({"demand", fmt("%d cells (target + 2 overview levels), plan #%llu",
                                      t.demand, static_cast<unsigned long long>(t.generation))});
    if (!s.tile_error.empty()) {
      sec.rows.push_back({"first error", s.tile_error, StatusTone::Bad});
    }
    out.push_back(std::move(sec));

    TileLoader::Stats const& l = t.loader;
    StatusSection ls{"Tile loader (all views of this path)", {}};
    ls.rows.push_back({"cells", fmt("%d ready, %d queued, %d failed, %d unavailable", l.ready,
                                    l.queued, l.failed, l.unavailable)});
    ls.rows.push_back({"requests", fmt("%llu issued, %llu answered, %llu stale dropped, %llu "
                                       "cancelled, %llu evicted",
                                       static_cast<unsigned long long>(l.issued_total),
                                       static_cast<unsigned long long>(l.results_total),
                                       static_cast<unsigned long long>(l.stale_results),
                                       static_cast<unsigned long long>(l.cancels_sent),
                                       static_cast<unsigned long long>(l.evicted))});
    if (l.stalls > 0) {
      ls.rows.push_back({"stalls", fmt("%llu requests got no answer within the watchdog",
                                       static_cast<unsigned long long>(l.stalls)),
                         StatusTone::Bad});
    }
    if (l.content_changes > 0) {
      ls.rows.push_back({"size changes", fmt("%d (%s)", l.content_changes,
                                             l.content_change_note.c_str()),
                         StatusTone::Warn});
    }
    if (!l.last_error.empty()) {
      ls.rows.push_back({"last error", l.last_error, StatusTone::Warn});
    }
    out.push_back(std::move(ls));
  }

  if (s.render) {
    thumtoo::PdfDocumentRenderStats const& d = *s.render;
    StatusSection sec{"Rendering (thumtoo, this page)", {}};
    thumtoo::PdfPageRenderStats const* page = nullptr;
    for (auto const& p : d.pages) {
      if (p.page == r.page) {
        page = &p;
      }
    }
    if (!page) {
      sec.rows.push_back({"page", "not rendered yet"});
    } else {
      if (mupdf) {
      sec.rows.push_back({"display list", fmt("built %lld× (%.1f ms)",
                                              static_cast<long long>(page->display_list_builds),
                                              page->display_list_ms),
                          page->display_list_builds > 1 ? StatusTone::Warn : StatusTone::Normal});
      }
      double const avg = page->cells_rendered > 0
                             ? page->render_ms / static_cast<double>(page->cells_rendered)
                             : 0.0;
      sec.rows.push_back({"cells", fmt("%lld rendered (avg %.1f ms), %lld refused, %lld failed",
                                       static_cast<long long>(page->cells_rendered), avg,
                                       static_cast<long long>(page->cells_refused),
                                       static_cast<long long>(page->cells_failed)),
                          page->cells_failed > 0 ? StatusTone::Bad : StatusTone::Normal});
      if (page->page_rasters > 0) {
        sec.rows.push_back({"page rasters", fmt("%lld whole-page renders (thumbnails)",
                                                static_cast<long long>(page->page_rasters))});
      }
      add_decode_rows(sec.rows, page->decode, page->cells_rendered + page->page_rasters,
                      r.kind == SourceKind::DjvuPage);
      if (!page->last_error.empty()) {
        sec.rows.push_back({"last error", page->last_error, StatusTone::Bad});
      }
    }
    out.push_back(std::move(sec));

    StatusSection ds{"Rendering (thumtoo, whole document)", {}};
    ds.rows.push_back({"opens", fmt("%lld", static_cast<long long>(d.opens)),
                       d.opens > 1 ? StatusTone::Warn : StatusTone::Normal});
    ds.rows.push_back({"pages touched", fmt("%zu", d.pages.size())});
    std::int64_t cells = 0;
    for (auto const& p : d.pages) {
      cells += p.cells_rendered + p.page_rasters;
    }
    add_decode_rows(ds.rows, d.decode, cells, r.kind == SourceKind::DjvuPage);
    if (d.lock_waits > 0) {
      ds.rows.push_back({"document lock",
                         fmt("%lld waits, %.1f ms total, %.1f ms longest",
                             static_cast<long long>(d.lock_waits), d.lock_wait_ms,
                             d.lock_wait_max_ms)});
    }
    out.push_back(std::move(ds));
  }
  return out;
}

std::string format_status_text(std::vector<StatusSection> const& sections)
{
  std::string out;
  for (StatusSection const& sec : sections) {
    out += sec.title;
    out += '\n';
    for (StatusRow const& row : sec.rows) {
      out += "  ";
      out += row.label;
      out += ": ";
      out += row.value;
      if (row.tone == StatusTone::Warn) {
        out += "  [warn]";
      } else if (row.tone == StatusTone::Bad) {
        out += "  [problem]";
      }
      out += '\n';
    }
  }
  return out;
}

}  // namespace thumtoo::lod
