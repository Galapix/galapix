// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

// DjVu backend (djvulibre ddjvuapi).
//
// Runtime model (TILES.md "DjVu rendering"):
//   * One cached document per file (DjvuDoc, LRU kDjvuDocumentCacheSize),
//     each with its own ddjvu context and mutex — a ddjvu context is not
//     thread-safe, but different books render in parallel.
//   * Decoded pages stay alive per document (LRU kDjvuPageCacheSize); every
//     tile cell of a page renders from the same decoded page. Each decode is
//     counted (render stats).
//   * Layout (scale 0) is the page's native pixel grid. DjVu pages are
//     rasters: finer scales are refused (Unavailable) instead of interpolated.
//   * Decoder errors arrive as ddjvu messages; they are kept and reported.

#include "thumtoo/djvu.hpp"

#include "thumtoo/format.hpp"
#include "thumtoo/image.hpp"
#include "thumtoo/uri.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <string>

#if defined(THUMTOO_HAVE_DJVU)
#  include <libdjvu/ddjvuapi.h>
#  include <libdjvu/miniexp.h>
#endif

namespace thumtoo {
namespace {

constexpr std::string_view kPagePipe = "//page:";

#if defined(THUMTOO_HAVE_DJVU)

constexpr int kDjvuDocumentCacheSize = 4;
constexpr int kDjvuPageCacheSize = 4;

using Clock = std::chrono::steady_clock;

double ms_since(Clock::time_point t0) {
  return std::chrono::duration<double, std::milli>(Clock::now() - t0).count();
}

// --- Render / decode accounting ---------------------------------------------

struct StatsRegistry {
  std::mutex mu;
  std::map<std::string, PdfDocumentRenderStats> docs;
};

StatsRegistry& stats_registry() {
  static auto* r = new StatsRegistry;
  return *r;
}

template <class F>
void with_doc_stats(const std::string& key, F&& f) {
  auto& r = stats_registry();
  std::lock_guard lock(r.mu);
  auto& d = r.docs[key];
  if (d.path.empty()) d.path = key;
  f(d);
}

template <class F>
void with_page_stats(const std::string& key, int page, F&& f) {
  auto& r = stats_registry();
  std::lock_guard lock(r.mu);
  auto& d = r.docs[key];
  if (d.path.empty()) d.path = key;
  auto it = std::lower_bound(d.pages.begin(), d.pages.end(), page,
                             [](const PdfPageRenderStats& p, int v) { return p.page < v; });
  if (it == d.pages.end() || it->page != page) {
    PdfPageRenderStats fresh;
    fresh.page = page;
    it = d.pages.insert(it, std::move(fresh));
  }
  f(d, *it);
}

// --- Document / page cache ---------------------------------------------------

struct PageSlot {
  ddjvu_page_t* page = nullptr;
  int width = 0;
  int height = 0;
  int dpi = 0;
  std::optional<PdfPageProfile> profile;
  std::uint64_t last_use = 0;
};

struct DjvuDoc {
  std::string key;  // normalized path
  std::filesystem::path path;
  std::filesystem::file_time_type mtime{};

  std::mutex mu;  // guards everything below (ddjvu context is single-threaded)
  bool open_attempted = false;
  std::string open_error;
  ddjvu_context_t* ctx = nullptr;
  ddjvu_document_t* doc = nullptr;
  std::map<int, PageSlot> pages;  // 0-based
  std::uint64_t clock = 0;
  std::string last_error;  // latest DDJVU_ERROR message text

  void pump() {
    if (!ctx) return;
    const ddjvu_message_t* msg;
    while ((msg = ddjvu_message_peek(ctx)) != nullptr) {
      if (msg->m_any.tag == DDJVU_ERROR && msg->m_error.message) {
        last_error = msg->m_error.message;
      }
      ddjvu_message_pop(ctx);
    }
  }

  void wait() {
    ddjvu_message_wait(ctx);
    pump();
  }

  ~DjvuDoc() {
    for (auto& [idx, s] : pages) {
      if (s.page) ddjvu_page_release(s.page);
    }
    if (doc) ddjvu_document_release(doc);
    if (ctx) ddjvu_context_release(ctx);
  }
};

struct DocCache {
  std::mutex mu;
  std::list<std::shared_ptr<DjvuDoc>> lru;  // front = most recent
};

DocCache& doc_cache() {
  static auto* c = new DocCache;
  return *c;
}

std::shared_ptr<DjvuDoc> cache_entry(const std::filesystem::path& path) {
  std::error_code ec;
  const auto mtime = std::filesystem::last_write_time(path, ec);
  const std::string key = path.lexically_normal().string();
  auto& c = doc_cache();
  std::lock_guard lock(c.mu);
  for (auto it = c.lru.begin(); it != c.lru.end(); ++it) {
    if ((*it)->key != key) continue;
    if (!ec && (*it)->mtime == mtime) {
      auto e = *it;
      c.lru.erase(it);
      c.lru.push_front(e);
      return e;
    }
    c.lru.erase(it);  // file changed: reopen
    break;
  }
  auto e = std::make_shared<DjvuDoc>();
  e->key = key;
  e->path = path;
  e->mtime = ec ? std::filesystem::file_time_type{} : mtime;
  c.lru.push_front(e);
  while (static_cast<int>(c.lru.size()) > kDjvuDocumentCacheSize) {
    c.lru.pop_back();  // users keep their shared_ptr
  }
  return e;
}

// ddjvu_context_create calls setlocale() (process-global): serialize it.
std::mutex g_context_create_mu;

void open_locked(DjvuDoc& e) {
  e.open_attempted = true;
  {
    std::lock_guard lock(g_context_create_mu);
    e.ctx = ddjvu_context_create("thumtoo");
  }
  if (!e.ctx) {
    e.open_error = "ddjvu_context_create failed";
    return;
  }
  // Decoded data we keep lives in our page slots; the context cache would
  // only duplicate it.
  ddjvu_cache_set_size(e.ctx, 0);
  e.doc = ddjvu_document_create_by_filename_utf8(e.ctx, e.path.string().c_str(), TRUE);
  if (!e.doc) {
    e.open_error = "cannot open " + e.path.string();
    return;
  }
  while (!ddjvu_document_decoding_done(e.doc)) e.wait();
  if (ddjvu_document_decoding_error(e.doc)) {
    e.open_error = e.last_error.empty() ? "document decoding failed" : e.last_error;
    ddjvu_document_release(e.doc);
    e.doc = nullptr;
    return;
  }
  with_doc_stats(e.key, [](PdfDocumentRenderStats& d) { ++d.opens; });
}

/// Exclusive access to one cached document for the lifetime of the object.
class DjvuAccess {
 public:
  explicit DjvuAccess(const std::filesystem::path& path) : entry_(cache_entry(path)) {
    lock_ = std::unique_lock<std::mutex>(entry_->mu, std::try_to_lock);
    if (!lock_.owns_lock()) {
      const Clock::time_point t0 = Clock::now();
      lock_.lock();
      const double waited = ms_since(t0);
      with_doc_stats(entry_->key, [&](PdfDocumentRenderStats& d) {
        ++d.lock_waits;
        d.lock_wait_ms += waited;
        d.lock_wait_max_ms = std::max(d.lock_wait_max_ms, waited);
      });
    }
    if (!entry_->open_attempted) open_locked(*entry_);
    if (!entry_->doc) error_ = entry_->open_error;
  }

  explicit operator bool() const { return entry_->doc != nullptr; }
  DjvuDoc& doc() { return *entry_; }
  const std::string& error() const { return error_; }

  int page_count() {
    if (!*this) return 0;
    entry_->pump();
    return ddjvu_document_get_pagenum(entry_->doc);
  }

  /// Page size/dpi from the document directory (no page decode).
  std::optional<ddjvu_pageinfo_t> page_info(int page_1based) {
    if (!*this) return std::nullopt;
    if (page_1based < 1 || page_1based > page_count()) {
      error_ = "page " + std::to_string(page_1based) + " out of range (document has " +
               std::to_string(page_count()) + ")";
      return std::nullopt;
    }
    ddjvu_pageinfo_t info{};
    ddjvu_status_t st;
    while ((st = ddjvu_document_get_pageinfo(entry_->doc, page_1based - 1, &info)) <
           DDJVU_JOB_OK) {
      entry_->wait();
    }
    if (st != DDJVU_JOB_OK || info.width <= 0 || info.height <= 0) {
      error_ = entry_->last_error.empty() ? "page info unavailable" : entry_->last_error;
      return std::nullopt;
    }
    return info;
  }

  /// Decoded page (decoded once, kept), or nullptr with error().
  PageSlot* decoded(int page_1based);

 private:
  void build_profile(PageSlot& s, int page_1based);

  std::shared_ptr<DjvuDoc> entry_;
  std::unique_lock<std::mutex> lock_;
  std::string error_;
};

PageSlot* DjvuAccess::decoded(int page_1based) {
  if (!page_info(page_1based)) return nullptr;
  DjvuDoc& e = *entry_;
  const int idx = page_1based - 1;
  auto it = e.pages.find(idx);
  if (it != e.pages.end() && it->second.page) {
    it->second.last_use = ++e.clock;
    return &it->second;
  }
  const Clock::time_point t0 = Clock::now();
  e.last_error.clear();
  ddjvu_page_t* page = ddjvu_page_create_by_pageno(e.doc, idx);
  if (!page) {
    error_ = "cannot create page " + std::to_string(page_1based);
    return nullptr;
  }
  while (!ddjvu_page_decoding_done(page)) e.wait();
  const double ms = ms_since(t0);
  if (ddjvu_page_decoding_error(page)) {
    error_ = "page decoding failed" + (e.last_error.empty() ? "" : ": " + e.last_error);
    ddjvu_page_release(page);
    with_page_stats(e.key, page_1based, [&](PdfDocumentRenderStats&, PdfPageRenderStats& p) {
      p.last_error = error_;
    });
    return nullptr;
  }
  PageSlot& s = e.pages[idx];
  s.page = page;
  s.width = ddjvu_page_get_width(page);
  s.height = ddjvu_page_get_height(page);
  s.dpi = ddjvu_page_get_resolution(page);
  s.last_use = ++e.clock;
  const std::int64_t pixels = static_cast<std::int64_t>(s.width) * s.height;
  with_page_stats(e.key, page_1based, [&](PdfDocumentRenderStats& d, PdfPageRenderStats& p) {
    for (PdfDecodeStats* ds : {&p.decode, &d.decode}) {
      ++ds->decodes;
      ++ds->full_decodes;
      ds->decoded_pixels += pixels;
      ds->decode_ms += ms;
      ds->largest_decode_pixels = std::max(ds->largest_decode_pixels, pixels);
    }
  });
  build_profile(s, page_1based);
  // Keep at most kDjvuPageCacheSize decoded pages.
  while (static_cast<int>(e.pages.size()) > kDjvuPageCacheSize) {
    auto victim = e.pages.begin();
    for (auto v = e.pages.begin(); v != e.pages.end(); ++v) {
      if (v->second.last_use < victim->second.last_use) victim = v;
    }
    if (victim->second.page) ddjvu_page_release(victim->second.page);
    e.pages.erase(victim);
  }
  return &e.pages[idx];
}

/// "BG44 [7851]  IW4 data #1, 74 slices, v1.2 (color), 850x1100" → 850x1100.
bool layer_size_from_dump(const char* dump, const char* chunk, int* w, int* h,
                          bool* color) {
  if (!dump) return false;
  for (const char* line = std::strstr(dump, chunk); line;
       line = std::strstr(line + 1, chunk)) {
    const char* eol = std::strchr(line, '\n');
    const std::string text(line, eol ? static_cast<std::size_t>(eol - line) : std::strlen(line));
    const auto paren = text.rfind("), ");
    if (paren == std::string::npos) continue;
    if (std::sscanf(text.c_str() + paren + 3, "%dx%d", w, h) == 2) {
      *color = text.find("(color)") != std::string::npos;
      return true;
    }
  }
  return false;
}

int utf8_codepoints(const char* s) {
  int n = 0;
  for (; s && *s; ++s) {
    const unsigned char c = static_cast<unsigned char>(*s);
    if ((c & 0xC0) != 0x80 && c > ' ') ++n;
  }
  return n;
}

void DjvuAccess::build_profile(PageSlot& s, int page_1based) {
  DjvuDoc& e = *entry_;
  PdfPageProfile p;
  const double dpi = s.dpi > 0 ? s.dpi : 300.0;
  p.width_pt = s.width * 72.0 / dpi;
  p.height_pt = s.height * 72.0 / dpi;
  p.native_dpi = s.dpi;

  char* dump = nullptr;
  while ((dump = ddjvu_document_get_pagedump(e.doc, page_1based - 1)) == nullptr) {
    if (ddjvu_document_decoding_error(e.doc)) break;
    e.wait();
  }
  const bool has_mask = dump && std::strstr(dump, "Sjbz") != nullptr;
  int bw = 0, bh = 0;
  bool bcolor = false;
  const bool has_bg = layer_size_from_dump(dump, "BG44", &bw, &bh, &bcolor);
  int fw = 0, fh = 0;
  bool fcolor = false;
  const bool has_fg44 = layer_size_from_dump(dump, "FG44", &fw, &fh, &fcolor);
  std::free(dump);

  auto add = [&](int w, int h, int comps, int bpc, bool stencil) {
    PdfPageImage im;
    im.width = w;
    im.height = h;
    im.components = comps;
    im.bpc = bpc;
    im.stencil = stencil;
    im.dpi = s.width > 0 ? dpi * w / s.width : 0.0;
    im.page_fraction = 1.0;
    p.images.push_back(im);
  };
  if (has_mask) add(s.width, s.height, 1, 1, true);
  if (has_bg) add(bw, bh, bcolor ? 3 : 1, 8, false);
  if (has_fg44) add(fw, fh, fcolor ? 3 : 1, 8, false);
  p.image_draws = static_cast<int>(p.images.size());
  p.image_coverage = p.images.empty() ? 0.0 : 1.0;

  miniexp_t text = miniexp_dummy;
  while ((text = ddjvu_document_get_pagetext(e.doc, page_1based - 1, "page")) ==
         miniexp_dummy) {
    e.wait();
  }
  if (text && text != miniexp_nil && miniexp_consp(text)) {
    for (miniexp_t q = text; miniexp_consp(q); q = miniexp_cdr(q)) {
      if (miniexp_stringp(miniexp_car(q))) {
        p.invisible_glyphs += utf8_codepoints(miniexp_to_str(miniexp_car(q)));
      }
    }
    ddjvu_miniexp_release(e.doc, text);
  }

  char buf[256];
  const char* type = "page";
  switch (ddjvu_page_get_type(s.page)) {
    case DDJVU_PAGETYPE_BITONAL: type = "bitonal"; break;
    case DDJVU_PAGETYPE_PHOTO: type = "photo"; break;
    case DDJVU_PAGETYPE_COMPOUND: type = "compound"; break;
    default: break;
  }
  if (p.images.empty()) {
    p.kind = PageContentKind::Empty;
    p.summary = "empty: DjVu page without image layers (renders white)";
  } else {
    p.kind = PageContentKind::Raster;
    p.finest_useful_scale = 0;
    std::snprintf(buf, sizeof buf,
                  "raster: DjVu %s page %dx%d at %d dpi, %d layer%s", type, s.width,
                  s.height, s.dpi, p.image_draws, p.image_draws == 1 ? "" : "s");
    p.summary = buf;
    if (p.invisible_glyphs > 0) {
      std::snprintf(buf, sizeof buf, "; hidden text layer (%d glyphs)", p.invisible_glyphs);
      p.summary += buf;
    }
    p.summary += " → finest useful scale 0 (native page pixels)";
  }
  s.profile = p;
  with_page_stats(e.key, page_1based, [&](PdfDocumentRenderStats&, PdfPageRenderStats& ps) {
    ps.profile = p;
  });
}

/// Render @p rect of the page scaled to @p full_w x @p full_h (exact grid).
/// Caller holds the document lock.
std::optional<DjvuRaster> render_rect(DjvuDoc& e, PageSlot& s, int full_w, int full_h,
                                      int px, int py, int pw, int ph, std::string* error) {
  ddjvu_rect_t pagerect{0, 0, static_cast<unsigned>(full_w), static_cast<unsigned>(full_h)};
  ddjvu_rect_t renderrect{px, py, static_cast<unsigned>(pw), static_cast<unsigned>(ph)};
  ddjvu_format_t* fmt = ddjvu_format_create(DDJVU_FORMAT_RGB24, 0, nullptr);
  if (!fmt) {
    if (error) *error = "ddjvu_format_create failed";
    return std::nullopt;
  }
  // Image-style coordinates: y grows downward (the tile grid's convention).
  ddjvu_format_set_row_order(fmt, 1);
  ddjvu_format_set_y_direction(fmt, 1);
  DjvuRaster out;
  out.width = pw;
  out.height = ph;
  out.rgb.resize(static_cast<std::size_t>(pw) * static_cast<std::size_t>(ph) * 3u);
  e.last_error.clear();
  const int ok = ddjvu_page_render(s.page, DDJVU_RENDER_COLOR, &pagerect, &renderrect, fmt,
                                   pw * 3, reinterpret_cast<char*>(out.rgb.data()));
  ddjvu_format_release(fmt);
  e.pump();
  if (!ok) {
    // FALSE on a fully decoded page means it has no image layer at all (the
    // profile says Empty): white is the true content. Anything else failed.
    if (s.profile && s.profile->kind == PageContentKind::Empty && e.last_error.empty()) {
      std::fill(out.rgb.begin(), out.rgb.end(), static_cast<std::uint8_t>(255));
      return out;
    }
    if (error) {
      *error = "render failed" + (e.last_error.empty() ? "" : ": " + e.last_error);
    }
    return std::nullopt;
  }
  return out;
}

#endif  // THUMTOO_HAVE_DJVU

}  // namespace

bool is_likely_djvu_path(const std::filesystem::path& path) {
  return is_djvu_path(path);
}

std::string djvu_page_uri(const std::filesystem::path& djvu_path,
                          int page_1based) {
  auto uri = file_uri_from_path(djvu_path.lexically_normal());
  uri += "//page:";
  uri += std::to_string(std::max(1, page_1based));
  return uri;
}

std::optional<ParsedDjvuUri> parse_djvu_uri(std::string_view uri) {
  const auto pipe = uri.find(kPagePipe);
  if (pipe == std::string_view::npos) return std::nullopt;

  const auto outer = uri.substr(0, pipe);
  auto path = path_from_file_uri(outer);
  if (!path) return std::nullopt;
  if (!is_djvu_path(*path)) return std::nullopt;

  std::string_view rest = uri.substr(pipe + kPagePipe.size());
  if (rest.empty()) return std::nullopt;
  int page = 0;
  for (char c : rest) {
    if (c < '0' || c > '9') return std::nullopt;
    page = page * 10 + (c - '0');
    if (page > 1'000'000) return std::nullopt;
  }
  if (page < 1) return std::nullopt;

  ParsedDjvuUri out;
  out.djvu_path = *path;
  out.page = page;
  return out;
}

std::optional<int> djvu_page_count(const std::filesystem::path& path) {
#if !defined(THUMTOO_HAVE_DJVU)
  (void)path;
  return std::nullopt;
#else
  DjvuAccess acc(path);
  const int n = acc.page_count();
  if (n <= 0) return std::nullopt;
  return n;
#endif
}

std::optional<Size> djvu_page_size_native(const std::filesystem::path& path,
                                          int page_1based) {
#if !defined(THUMTOO_HAVE_DJVU)
  (void)path;
  (void)page_1based;
  return std::nullopt;
#else
  DjvuAccess acc(path);
  auto info = acc.page_info(page_1based);
  if (!info) return std::nullopt;
  return Size{info->width, info->height};
#endif
}

std::optional<Size> djvu_page_layout_size(const std::filesystem::path& path,
                                          int page_1based) {
  // Native decoder pixels are the layout grid (scale 0). DjVu pages are
  // already pixel-sized (often ~200–300 dpi scans), unlike PDF media points.
  return djvu_page_size_native(path, page_1based);
}

Size djvu_page_size_at_scale(Size layout, int scale) {
  if (layout.width <= 0 || layout.height <= 0) return Size{0, 0};
  if (scale < 0) {
    const int mul = 1 << (-scale);
    return Size{layout.width * mul, layout.height * mul};
  }
  // Successive floor-half: the host's tile grid (dim_at_tile_scale).
  return Size{dim_at_tile_scale(layout.width, scale),
              dim_at_tile_scale(layout.height, scale)};
}

std::optional<DjvuRaster> djvu_rasterize_page(const std::filesystem::path& path,
                                              int page_1based, int max_edge) {
#if !defined(THUMTOO_HAVE_DJVU)
  (void)path;
  (void)page_1based;
  (void)max_edge;
  return std::nullopt;
#else
  DjvuAccess acc(path);
  PageSlot* s = acc.decoded(page_1based);
  if (!s || s->width <= 0 || s->height <= 0) return std::nullopt;
  int out_w = s->width;
  int out_h = s->height;
  if (max_edge > 0 && std::max(out_w, out_h) > max_edge) {
    const double f = static_cast<double>(max_edge) / std::max(out_w, out_h);
    out_w = std::max(1, static_cast<int>(std::lround(out_w * f)));
    out_h = std::max(1, static_cast<int>(std::lround(out_h * f)));
  }
  std::string error;
  auto out = render_rect(acc.doc(), *s, out_w, out_h, 0, 0, out_w, out_h, &error);
  with_page_stats(acc.doc().key, page_1based,
                  [&](PdfDocumentRenderStats&, PdfPageRenderStats& p) {
                    ++p.page_rasters;
                    if (!out) p.last_error = error;
                  });
  return out;
#endif
}

std::optional<DjvuRaster> djvu_rasterize_page_region(
    const std::filesystem::path& path, int page_1based, double scale_factor,
    int px, int py, int pw, int ph) {
#if !defined(THUMTOO_HAVE_DJVU)
  (void)path;
  (void)page_1based;
  (void)scale_factor;
  (void)px;
  (void)py;
  (void)pw;
  (void)ph;
  return std::nullopt;
#else
  if (pw <= 0 || ph <= 0) return std::nullopt;
  DjvuAccess acc(path);
  PageSlot* s = acc.decoded(page_1based);
  if (!s) return std::nullopt;
  const double f = scale_factor > 0.0 ? scale_factor : 1.0;
  const int full_w = std::max(1, static_cast<int>(std::lround(s->width * f)));
  const int full_h = std::max(1, static_cast<int>(std::lround(s->height * f)));
  return render_rect(acc.doc(), *s, full_w, full_h, px, py, pw, ph, nullptr);
#endif
}

DjvuCellRender djvu_render_tile_cell(const std::filesystem::path& path,
                                     int page_1based, int scale, int x, int y) {
  DjvuCellRender out;
#if !defined(THUMTOO_HAVE_DJVU)
  (void)path;
  (void)page_1based;
  (void)scale;
  (void)x;
  (void)y;
  out.error = "DjVu backend unavailable";
  return out;
#else
  DjvuAccess acc(path);
  if (!acc) {
    out.error = acc.error();
    return out;
  }
  const std::string key = acc.doc().key;
  if (scale < 0) {
    with_page_stats(key, page_1based, [](PdfDocumentRenderStats&, PdfPageRenderStats& p) {
      ++p.cells_refused;
    });
    out.status = TileStatus::Unavailable;
    out.error = "scale " + std::to_string(scale) +
                " is finer than the DjVu page's native pixels (finest useful scale 0)";
    return out;
  }
  PageSlot* s = acc.decoded(page_1based);
  if (!s) {
    out.error = acc.error();
    with_page_stats(key, page_1based, [](PdfDocumentRenderStats&, PdfPageRenderStats& p) {
      ++p.cells_failed;
    });
    return out;
  }
  const Size full = djvu_page_size_at_scale(Size{s->width, s->height}, scale);
  int x0 = 0, y0 = 0, pw = 0, ph = 0;
  tile_cell_pixel_rect(full.width, full.height, x, y, &x0, &y0, &pw, &ph);
  if (pw <= 0 || ph <= 0) {
    out.status = TileStatus::Unavailable;
    out.error = "cell " + std::to_string(x) + "," + std::to_string(y) + " outside the " +
                std::to_string(full.width) + "x" + std::to_string(full.height) +
                " page at scale " + std::to_string(scale);
    return out;
  }
  const Clock::time_point t0 = Clock::now();
  std::string error;
  auto raster = render_rect(acc.doc(), *s, full.width, full.height, x0, y0, pw, ph, &error);
  const double ms = ms_since(t0);
  with_page_stats(key, page_1based, [&](PdfDocumentRenderStats&, PdfPageRenderStats& p) {
    if (raster) {
      ++p.cells_rendered;
      p.render_ms += ms;
    } else {
      ++p.cells_failed;
      p.last_error = error;
    }
  });
  if (!raster) {
    out.error = error;
    return out;
  }
  out.status = TileStatus::Ok;
  out.raster = std::move(raster);
  return out;
#endif
}

std::optional<TileBlob> djvu_build_tile_cell(const std::filesystem::path& path,
                                             int page_1based, int scale, int x,
                                             int y, int jpeg_quality) {
  auto cell = djvu_render_tile_cell(path, page_1based, scale, x, y);
  if (!cell.raster) return std::nullopt;
  return encode_tile_cell_rgb(cell.raster->rgb.data(), cell.raster->width,
                              cell.raster->height, scale, x, y, jpeg_quality);
}

std::optional<PdfPageProfile> djvu_page_profile(const std::filesystem::path& path,
                                                int page_1based, std::string* error) {
#if !defined(THUMTOO_HAVE_DJVU)
  (void)path;
  (void)page_1based;
  if (error) *error = "DjVu backend unavailable";
  return std::nullopt;
#else
  DjvuAccess acc(path);
  PageSlot* s = acc.decoded(page_1based);
  if (!s || !s->profile) {
    if (error) *error = acc.error().empty() ? "no profile" : acc.error();
    return std::nullopt;
  }
  return s->profile;
#endif
}

std::optional<PdfDocumentRenderStats> djvu_document_render_stats(
    const std::filesystem::path& path) {
#if !defined(THUMTOO_HAVE_DJVU)
  (void)path;
  return std::nullopt;
#else
  const std::string key = path.lexically_normal().string();
  auto& r = stats_registry();
  std::lock_guard lock(r.mu);
  auto it = r.docs.find(key);
  if (it == r.docs.end()) return std::nullopt;
  return it->second;
#endif
}

void djvu_reset_render_stats() {
#if defined(THUMTOO_HAVE_DJVU)
  auto& r = stats_registry();
  std::lock_guard lock(r.mu);
  r.docs.clear();
#endif
}

void djvu_release_document_cache() {
#if defined(THUMTOO_HAVE_DJVU)
  std::list<std::shared_ptr<DjvuDoc>> drop;
  {
    auto& c = doc_cache();
    std::lock_guard lock(c.mu);
    drop.swap(c.lru);
  }
#endif
}

#if defined(THUMTOO_HAVE_DJVU)

// DjVu text zones are nested miniexps:
//   (word|line|para|... xmin ymin xmax ymax "text" | child…)
// Coordinates are page pixels, origin bottom-left.
void walk_text_sexpr(miniexp_t expr, std::vector<TextRegion>& regions,
                     int page_h) {
  if (!miniexp_consp(expr)) return;
  miniexp_t head = miniexp_car(expr);
  if (!miniexp_symbolp(head)) return;

  const char* sym = miniexp_to_name(head);
  if (!sym) return;

  // Expect at least type + 4 numbers.
  miniexp_t rest = miniexp_cdr(expr);
  if (!miniexp_consp(rest)) return;
  miniexp_t x0e = miniexp_car(rest); rest = miniexp_cdr(rest);
  if (!miniexp_consp(rest)) return;
  miniexp_t y0e = miniexp_car(rest); rest = miniexp_cdr(rest);
  if (!miniexp_consp(rest)) return;
  miniexp_t x1e = miniexp_car(rest); rest = miniexp_cdr(rest);
  if (!miniexp_consp(rest)) return;
  miniexp_t y1e = miniexp_car(rest); rest = miniexp_cdr(rest);

  if (!miniexp_numberp(x0e) || !miniexp_numberp(y0e) ||
      !miniexp_numberp(x1e) || !miniexp_numberp(y1e)) {
    return;
  }
  const double x0 = static_cast<double>(miniexp_to_int(x0e));
  const double y0 = static_cast<double>(miniexp_to_int(y0e));
  const double x1 = static_cast<double>(miniexp_to_int(x1e));
  const double y1 = static_cast<double>(miniexp_to_int(y1e));

  const bool is_word = std::strcmp(sym, "word") == 0;
  const bool is_line = std::strcmp(sym, "line") == 0;
  // Prefer word-level; fall back to line if children are only a string.
  if (is_word || is_line) {
    // Collect trailing string leaves as the text for this zone.
    std::string text;
    for (miniexp_t p = rest; miniexp_consp(p); p = miniexp_cdr(p)) {
      miniexp_t el = miniexp_car(p);
      if (miniexp_stringp(el)) {
        const char* s = miniexp_to_str(el);
        if (s && s[0]) {
          if (!text.empty()) text.push_back(' ');
          text += s;
        }
      } else if (miniexp_consp(el)) {
        walk_text_sexpr(el, regions, page_h);
      }
    }
    if (is_word && !text.empty()) {
      TextRegion reg;
      reg.role = TextRegionRole::Text;
      reg.text = std::move(text);
      // Keep native bottom-left origin; document for consumers.
      reg.bbox = TextRect{x0, y0, x1, y1};
      if (!reg.bbox.empty()) regions.push_back(std::move(reg));
      return;
    }
    if (is_line && !text.empty()) {
      // Line with only a string (no word children).
      TextRegion reg;
      reg.role = TextRegionRole::Text;
      reg.text = std::move(text);
      reg.bbox = TextRect{x0, y0, x1, y1};
      if (!reg.bbox.empty()) regions.push_back(std::move(reg));
      return;
    }
  }

  // Recurse into children for page/column/region/para containers.
  for (miniexp_t p = rest; miniexp_consp(p); p = miniexp_cdr(p)) {
    miniexp_t el = miniexp_car(p);
    if (miniexp_consp(el)) walk_text_sexpr(el, regions, page_h);
  }
  (void)page_h;
}

void walk_anno_sexpr(miniexp_t expr, std::vector<TextRegion>& regions) {
  if (!miniexp_consp(expr)) return;
  miniexp_t head = miniexp_car(expr);
  if (miniexp_symbolp(head) &&
      std::strcmp(miniexp_to_name(head), "maparea") == 0) {
    // (maparea url_or_(url ... ) (rect x y w h) …) or (oval) (poly)
    std::string uri;
    TextRect bbox;
    bool have_bbox = false;
    for (miniexp_t p = miniexp_cdr(expr); miniexp_consp(p); p = miniexp_cdr(p)) {
      miniexp_t el = miniexp_car(p);
      if (miniexp_stringp(el)) {
        const char* s = miniexp_to_str(el);
        if (s) uri = s;
      } else if (miniexp_consp(el)) {
        miniexp_t eh = miniexp_car(el);
        if (!miniexp_symbolp(eh)) continue;
        const char* es = miniexp_to_name(eh);
        if (!es) continue;
        if (std::strcmp(es, "url") == 0) {
          miniexp_t u = miniexp_car(miniexp_cdr(el));
          if (miniexp_stringp(u)) {
            const char* s = miniexp_to_str(u);
            if (s) uri = s;
          }
        } else if (std::strcmp(es, "rect") == 0) {
          // (rect x y w h) — bottom-left origin
          miniexp_t a = miniexp_cdr(el);
          if (!miniexp_consp(a)) continue;
          int x = miniexp_numberp(miniexp_car(a)) ? miniexp_to_int(miniexp_car(a)) : 0;
          a = miniexp_cdr(a);
          if (!miniexp_consp(a)) continue;
          int y = miniexp_numberp(miniexp_car(a)) ? miniexp_to_int(miniexp_car(a)) : 0;
          a = miniexp_cdr(a);
          if (!miniexp_consp(a)) continue;
          int w = miniexp_numberp(miniexp_car(a)) ? miniexp_to_int(miniexp_car(a)) : 0;
          a = miniexp_cdr(a);
          if (!miniexp_consp(a)) continue;
          int h = miniexp_numberp(miniexp_car(a)) ? miniexp_to_int(miniexp_car(a)) : 0;
          if (w > 0 && h > 0) {
            bbox = TextRect{static_cast<double>(x), static_cast<double>(y),
                            static_cast<double>(x + w), static_cast<double>(y + h)};
            have_bbox = true;
          }
        }
      }
    }
    if (have_bbox && !uri.empty()) {
      TextRegion reg;
      reg.role = TextRegionRole::Link;
      reg.bbox = bbox;
      reg.target.kind = TextLinkTargetKind::Uri;
      reg.target.uri = std::move(uri);
      regions.push_back(std::move(reg));
    }
    return;
  }
  // Recurse lists of annotations.
  for (miniexp_t p = expr; miniexp_consp(p); p = miniexp_cdr(p)) {
    walk_anno_sexpr(miniexp_car(p), regions);
  }
}

void walk_outline_sexpr(miniexp_t expr, int level, DocumentOutline& out) {
  // Bookmarks: (bookmarks (title url …) (title url (children…)) …)
  // or nested lists of (title dest [children])
  if (!miniexp_consp(expr)) return;
  miniexp_t head = miniexp_car(expr);
  if (miniexp_symbolp(head) &&
      std::strcmp(miniexp_to_name(head), "bookmarks") == 0) {
    for (miniexp_t p = miniexp_cdr(expr); miniexp_consp(p); p = miniexp_cdr(p)) {
      walk_outline_sexpr(miniexp_car(p), level, out);
    }
    return;
  }
  // Entry: (title dest child…) where title and dest are strings.
  if (miniexp_stringp(head)) {
    OutlineItem item;
    item.level = level;
    const char* title = miniexp_to_str(head);
    if (title) item.title = title;
    miniexp_t rest = miniexp_cdr(expr);
    if (miniexp_consp(rest) && miniexp_stringp(miniexp_car(rest))) {
      const char* dest = miniexp_to_str(miniexp_car(rest));
      if (dest && dest[0]) {
        // "#N" page dest or URL
        if (dest[0] == '#') {
          int page = 0;
          if (std::sscanf(dest + 1, "%d", &page) == 1 && page >= 1) {
            item.page_1based = page;
          } else {
            item.uri = dest;
          }
        } else {
          item.uri = dest;
        }
      }
      rest = miniexp_cdr(rest);
    }
    out.items.push_back(std::move(item));
    for (; miniexp_consp(rest); rest = miniexp_cdr(rest)) {
      walk_outline_sexpr(miniexp_car(rest), level + 1, out);
    }
    return;
  }
  for (miniexp_t p = expr; miniexp_consp(p); p = miniexp_cdr(p)) {
    walk_outline_sexpr(miniexp_car(p), level, out);
  }
}

#endif  // THUMTOO_HAVE_DJVU


std::optional<PageTextLayer> djvu_page_text_layer(const std::filesystem::path& path,
                                                  int page_1based) {
#if !defined(THUMTOO_HAVE_DJVU)
  (void)path;
  (void)page_1based;
  return std::nullopt;
#else
  DjvuAccess acc(path);
  auto info = acc.page_info(page_1based);
  if (!info) return std::nullopt;
  DjvuDoc& e = acc.doc();

  PageTextLayer layer;
  layer.page_1based = page_1based;
  layer.page_bounds = TextRect{0, 0, static_cast<double>(info->width),
                               static_cast<double>(info->height)};
  layer.page_y_up = true;  // DjVu text zones: bottom-left origin

  miniexp_t text = miniexp_dummy;
  while ((text = ddjvu_document_get_pagetext(e.doc, page_1based - 1, "word")) ==
         miniexp_dummy) {
    e.wait();
  }
  if (text && text != miniexp_nil && !miniexp_symbolp(text)) {
    walk_text_sexpr(text, layer.regions, info->height);
    ddjvu_miniexp_release(e.doc, text);
  }

  miniexp_t anno = miniexp_dummy;
  while ((anno = ddjvu_document_get_pageanno(e.doc, page_1based - 1)) == miniexp_dummy) {
    e.wait();
  }
  if (anno && anno != miniexp_nil && !miniexp_symbolp(anno)) {
    walk_anno_sexpr(anno, layer.regions);
    ddjvu_miniexp_release(e.doc, anno);
  }
  return layer;
#endif
}

std::optional<DocumentOutline> djvu_document_outline(
    const std::filesystem::path& path) {
#if !defined(THUMTOO_HAVE_DJVU)
  (void)path;
  return std::nullopt;
#else
  DjvuAccess acc(path);
  if (!acc) return std::nullopt;
  DjvuDoc& e = acc.doc();
  miniexp_t root = miniexp_dummy;
  while ((root = ddjvu_document_get_outline(e.doc)) == miniexp_dummy) {
    e.wait();
  }
  DocumentOutline out;
  if (root && root != miniexp_nil && !miniexp_symbolp(root)) {
    walk_outline_sexpr(root, 1, out);
    ddjvu_miniexp_release(e.doc, root);
  }
  return out;
#endif
}

}  // namespace thumtoo
