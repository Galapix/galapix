// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

// MuPDF backend. Built only when THUMTOO_HAVE_MUPDF (see pdf_mupdf_stub.cpp).
//
// Runtime model (docs: TILES.md "PDF rendering"):
//   * One base fz_context with locks owns the shared store (decoded images,
//     glyphs). Every thread renders with its own fz_clone_context.
//   * One fz_document per file (DocEntry), guarded by a mutex: MuPDF documents
//     are single-threaded. Each page keeps a display list built once through a
//     profiling device, which records what the page draws (PdfPageProfile) and
//     wraps every image in a CountingImage.
//   * Cells run the shared display list outside the document lock, with the
//     cell as the device rect. No full-page rasters.
//   * CountingImage decodes the whole image once (when it fits
//     kPdfFullImageDecodeBudget) so every cell reuses the stored pixmap, and
//     counts each real decode into the render stats.

#include "thumtoo/pdf_mupdf.hpp"
#include "thumtoo/constants.hpp"
#include "thumtoo/format.hpp"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <tuple>
#include <atomic>
#include <vector>

#include <mupdf/fitz.h>
#include <mupdf/pdf.h>

namespace thumtoo {

/// Encode a Unicode code point as UTF-8. Skips surrogates and out-of-range
/// values (illegal in UTF-8) so QString::fromUtf8 never sees broken sequences.
void utf8_append_codepoint(std::string& out, int c) {
  if (c <= 0) return;
  if (c > 0x10FFFF) return;
  if (c >= 0xD800 && c <= 0xDFFF) return;  // UTF-16 surrogates — invalid in UTF-8
  if (c < 0x80) {
    out.push_back(static_cast<char>(c));
  } else if (c < 0x800) {
    out.push_back(static_cast<char>(0xC0 | (c >> 6)));
    out.push_back(static_cast<char>(0x80 | (c & 0x3F)));
  } else if (c < 0x10000) {
    out.push_back(static_cast<char>(0xE0 | (c >> 12)));
    out.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | (c & 0x3F)));
  } else {
    out.push_back(static_cast<char>(0xF0 | (c >> 18)));
    out.push_back(static_cast<char>(0x80 | ((c >> 12) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | (c & 0x3F)));
  }
}

namespace {

using Clock = std::chrono::steady_clock;

double ms_since(Clock::time_point t0) {
  return std::chrono::duration<double, std::milli>(Clock::now() - t0).count();
}

thread_local std::string g_force_text_key;
thread_local std::string g_last_mupdf_error;

std::atomic<bool> g_smooth_image_scaling{true};
std::atomic<std::size_t> g_full_decode_budget{kPdfFullImageDecodeBudget};

int tune_image_scale_cb(void* /*arg*/, int dst_w, int dst_h, int src_w,
                        int src_h)
{
  // MuPDF default: Mitchell only when downscaling. Smooth mode uses
  // Mitchell for upscales too (scan XObjects into denser tiles).
  if (g_smooth_image_scaling.load(std::memory_order_relaxed)) {
    return 1;
  }
  if (dst_w < src_w || dst_h < src_h) {
    return 1;
  }
  return 0;
}

// MuPDF routes .md through cmark (fz_htdoc_*). libcmark is not safe for
// concurrent open from multiple threads even with separate contexts.
std::mutex g_mupdf_markdown_mu;

struct MarkdownMuPdfLock {
  std::unique_lock<std::mutex> lock;
  explicit MarkdownMuPdfLock(const std::filesystem::path& path) {
    if (is_markdown_path(path)) {
      lock = std::unique_lock<std::mutex>(g_mupdf_markdown_mu);
    }
  }
};

void mupdf_store_message(const char* msg)
{
  if (!msg || !msg[0]) {
    return;
  }
  g_last_mupdf_error = msg;
  while (!g_last_mupdf_error.empty() &&
         (g_last_mupdf_error.back() == '\n' || g_last_mupdf_error.back() == '\r')) {
    g_last_mupdf_error.pop_back();
  }
}

void mupdf_error_cb(void* /*user*/, const char* msg) { mupdf_store_message(msg); }
void mupdf_warning_cb(void* /*user*/, const char* msg) { mupdf_store_message(msg); }

// --- Runtime: base context (shared store + locks), one clone per thread ----

struct Runtime {
  std::mutex locks[FZ_LOCK_MAX];
  fz_locks_context lc{};
  fz_context* base = nullptr;
};

void runtime_lock(void* user, int lock) {
  static_cast<Runtime*>(user)->locks[lock].lock();
}
void runtime_unlock(void* user, int lock) {
  static_cast<Runtime*>(user)->locks[lock].unlock();
}

Runtime* runtime() {
  // Never destroyed: clones and cached documents may be released during
  // static destruction in any order.
  static Runtime* rt = [] {
    auto* r = new Runtime;
    r->lc.user = r;
    r->lc.lock = runtime_lock;
    r->lc.unlock = runtime_unlock;
    fz_context* ctx = fz_new_context(nullptr, &r->lc, kPdfStoreBytes);
    if (ctx) {
      int ok = 0;
      fz_var(ok);
      fz_try(ctx) {
        fz_register_document_handlers(ctx);
        fz_tune_image_scale(ctx, tune_image_scale_cb, nullptr);
        ok = 1;
      }
      fz_catch(ctx) { ok = 0; }
      if (!ok) {
        fz_drop_context(ctx);
        ctx = nullptr;
      }
    }
    r->base = ctx;
    return r;
  }();
  return rt;
}

struct ThreadCtx {
  fz_context* ctx = nullptr;
  ~ThreadCtx() {
    if (ctx) fz_drop_context(ctx);
  }
};
thread_local ThreadCtx g_thread_ctx;

fz_context* thread_ctx() {
  if (!g_thread_ctx.ctx) {
    Runtime* rt = runtime();
    if (!rt->base) return nullptr;
    fz_context* ctx = fz_clone_context(rt->base);
    if (ctx) {
      // Keep messages for hosts instead of MuPDF's stderr spam.
      fz_set_error_callback(ctx, mupdf_error_cb, nullptr);
      fz_set_warning_callback(ctx, mupdf_warning_cb, nullptr);
    }
    g_thread_ctx.ctx = ctx;
  }
  return g_thread_ctx.ctx;
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

PdfPageRenderStats& page_stats_locked(PdfDocumentRenderStats& d, int page) {
  auto it = std::lower_bound(
      d.pages.begin(), d.pages.end(), page,
      [](const PdfPageRenderStats& p, int v) { return p.page < v; });
  if (it == d.pages.end() || it->page != page) {
    PdfPageRenderStats fresh;
    fresh.page = page;
    it = d.pages.insert(it, std::move(fresh));
  }
  return *it;
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
  with_doc_stats(key, [&](PdfDocumentRenderStats& d) { f(d, page_stats_locked(d, page)); });
}

void add_decode(PdfDecodeStats& s, bool full, std::int64_t pixels, double ms,
                const std::string& subarea_reason) {
  ++s.decodes;
  if (full) {
    ++s.full_decodes;
  } else {
    ++s.subarea_decodes;
    if (!subarea_reason.empty()) s.last_subarea_reason = subarea_reason;
  }
  s.decoded_pixels += pixels;
  s.decode_ms += ms;
  s.largest_decode_pixels = std::max(s.largest_decode_pixels, pixels);
}

// --- CountingImage: decode-once + exact decode accounting --------------------

/// Single-flight per image: concurrent cells needing the same decode wait for
/// the first thread instead of decoding again. The result is held only until
/// the last waiter took it; afterwards the shared store owns it.
struct DecodeFlight {
  fz_irect rect{};
  int l2 = 0;
  bool done = false;
  int waiters = 0;
  fz_pixmap* result = nullptr;  // own ref while waiters > 0
  fz_irect rect_out{};
  int l2_out = 0;
};

struct DecodeGate {
  std::mutex mu;
  std::condition_variable_any cv;
  std::list<DecodeFlight> flights;
};

struct CountingImage {
  fz_image super;
  fz_image* inner;
  std::string* doc_key;  // owned
  DecodeGate* gate;      // owned
  int page;
};

void record_decode(const CountingImage* ci, bool full, std::int64_t pixels, double ms,
                   bool over_budget, std::size_t bytes, std::size_t budget) {
  std::string reason;
  if (over_budget) {
    char buf[160];
    std::snprintf(buf, sizeof buf,
                  "image %dx%d needs %zu MiB decoded (> %zu MiB budget); "
                  "decoded per cell",
                  ci->inner->w, ci->inner->h, bytes >> 20, budget >> 20);
    reason = buf;
  }
  with_page_stats(*ci->doc_key, ci->page,
                  [&](PdfDocumentRenderStats& d, PdfPageRenderStats& p) {
                    add_decode(p.decode, full, pixels, ms, reason);
                    add_decode(d.decode, full, pixels, ms, reason);
                  });
}

void record_shared_wait(const CountingImage* ci) {
  with_page_stats(*ci->doc_key, ci->page,
                  [](PdfDocumentRenderStats& d, PdfPageRenderStats& p) {
                    ++p.decode.shared_waits;
                    ++d.decode.shared_waits;
                  });
}

bool same_rect(const fz_irect& a, const fz_irect& b) {
  return a.x0 == b.x0 && a.y0 == b.y0 && a.x1 == b.x1 && a.y1 == b.y1;
}

// Called by fz_get_pixmap_from_image after its store lookup missed. May be
// entered by several threads at once (shared display list). No C++ object
// with a destructor may be live across a call that can throw (longjmp).
fz_pixmap* counting_get_pixmap(fz_context* ctx, fz_image* img, fz_irect* subarea,
                               int w, int h, int* l2factor) {
  auto* ci = reinterpret_cast<CountingImage*>(img);
  fz_image* inner = ci->inner;
  if (inner->decoded || inner->scalable || !subarea || !l2factor) {
    // Already-decoded pixmap images / vector images: no decompression here.
    return inner->get_pixmap(ctx, inner, subarea, w, h, l2factor);
  }
  const int l2 = *l2factor;
  const std::size_t f = std::size_t{1} << l2;
  const std::size_t dw = (static_cast<std::size_t>(inner->w) + f - 1) >> l2;
  const std::size_t dh = (static_cast<std::size_t>(inner->h) + f - 1) >> l2;
  const std::size_t bytes = dw * dh * std::max<std::size_t>(1, inner->n);
  const std::size_t budget = g_full_decode_budget.load(std::memory_order_relaxed);
  bool over_budget = false;
  if (subarea->x0 > 0 || subarea->y0 > 0 || subarea->x1 < inner->w ||
      subarea->y1 < inner->h) {
    if (bytes <= budget) {
      // Decode the whole image once; the store keys it as the full image and
      // every later cell (any subarea) hits it.
      subarea->x0 = 0;
      subarea->y0 = 0;
      subarea->x1 = inner->w;
      subarea->y1 = inner->h;
    } else {
      over_budget = true;
    }
  }

  DecodeGate* gate = ci->gate;
  DecodeFlight* flight = nullptr;
  gate->mu.lock();
  for (DecodeFlight& fl : gate->flights) {
    if (fl.l2 != l2 || !same_rect(fl.rect, *subarea)) continue;
    ++fl.waiters;
    gate->cv.wait(gate->mu, [&fl] { return fl.done; });
    --fl.waiters;
    fz_pixmap* shared = fl.result ? fz_keep_pixmap(ctx, fl.result) : nullptr;
    if (shared) {
      *subarea = fl.rect_out;
      *l2factor = fl.l2_out;
    }
    if (fl.waiters == 0) {
      fz_drop_pixmap(ctx, fl.result);
      fl.result = nullptr;
      for (auto it = gate->flights.begin(); it != gate->flights.end(); ++it) {
        if (&*it == &fl) {
          gate->flights.erase(it);
          break;
        }
      }
    }
    gate->mu.unlock();
    if (shared) {
      record_shared_wait(ci);
      return shared;
    }
    gate->mu.lock();  // the other decode failed: try ourselves
    break;
  }
  gate->flights.emplace_back();
  flight = &gate->flights.back();
  flight->rect = *subarea;
  flight->l2 = l2;
  gate->mu.unlock();

  fz_pixmap* pix = nullptr;
  int failed = 0;
  int code = 0;
  char message[256] = {0};
  fz_var(pix);
  fz_var(failed);
  const Clock::time_point t0 = Clock::now();
  fz_try(ctx) { pix = inner->get_pixmap(ctx, inner, subarea, w, h, l2factor); }
  fz_catch(ctx) {
    failed = 1;
    code = fz_caught(ctx);
    std::snprintf(message, sizeof message, "%s", fz_caught_message(ctx));
  }
  const double ms = ms_since(t0);

  gate->mu.lock();
  flight->done = true;
  flight->rect_out = *subarea;
  flight->l2_out = *l2factor;
  if (flight->waiters > 0 && pix) {
    flight->result = fz_keep_pixmap(ctx, pix);
  } else if (flight->waiters == 0) {
    for (auto it = gate->flights.begin(); it != gate->flights.end(); ++it) {
      if (&*it == flight) {
        gate->flights.erase(it);
        break;
      }
    }
  }
  gate->cv.notify_all();
  gate->mu.unlock();

  if (failed) {
    fz_throw(ctx, code, "%s", message);
  }
  const bool full = subarea->x0 <= 0 && subarea->y0 <= 0 && subarea->x1 >= inner->w &&
                    subarea->y1 >= inner->h;
  const std::int64_t pixels =
      pix ? static_cast<std::int64_t>(fz_pixmap_width(ctx, pix)) * fz_pixmap_height(ctx, pix)
          : 0;
  record_decode(ci, full, pixels, ms, over_budget, bytes, budget);
  return pix;
}

std::size_t counting_get_size(fz_context* ctx, fz_image* img) {
  auto* ci = reinterpret_cast<CountingImage*>(img);
  return sizeof(CountingImage) + fz_image_size(ctx, ci->inner);
}

void counting_get_digest(fz_context* ctx, fz_image* img, unsigned char digest[16]) {
  auto* ci = reinterpret_cast<CountingImage*>(img);
  if (ci->inner->get_digest) {
    ci->inner->get_digest(ctx, ci->inner, digest);
  } else {
    std::memset(digest, 0, 16);
  }
}

void counting_drop(fz_context* ctx, fz_image* img) {
  auto* ci = reinterpret_cast<CountingImage*>(img);
  fz_drop_image(ctx, ci->inner);
  delete ci->doc_key;
  ci->doc_key = nullptr;
  delete ci->gate;  // no flights: the image is only dropped when unused
  ci->gate = nullptr;
}

/// New reference to a CountingImage around @p inner. Throws (fz) on OOM.
fz_image* new_counting_image(fz_context* ctx, fz_image* inner,
                             const std::string& doc_key, int page) {
  auto* key = new std::string(doc_key);
  auto* gate = new DecodeGate;
  CountingImage* ci = nullptr;
  fz_var(ci);
  fz_try(ctx) {
    ci = fz_new_derived_image(
        ctx, inner->w, inner->h, inner->bpc, inner->colorspace, inner->xres,
        inner->yres, inner->interpolate, inner->imagemask,
        inner->use_decode ? inner->decode : nullptr,
        inner->use_colorkey ? inner->colorkey : nullptr, inner->mask,
        CountingImage, counting_get_pixmap, counting_get_size,
        counting_get_digest, counting_drop);
  }
  fz_catch(ctx) {
    delete key;
    delete gate;
    fz_rethrow(ctx);
  }
  ci->inner = fz_keep_image(ctx, inner);
  ci->doc_key = key;
  ci->gate = gate;
  ci->page = page;
  // Mirror the fields fz_new_image_of_size does not take.
  std::memcpy(ci->super.decode, inner->decode, sizeof inner->decode);
  ci->super.use_decode = inner->use_decode;
  ci->super.decoded = inner->decoded;
  ci->super.scalable = inner->scalable;
  ci->super.intent = inner->intent;
  ci->super.has_intent = inner->has_intent;
  ci->super.orientation = inner->orientation;
  return &ci->super;
}

// --- Profiling pass-through device ------------------------------------------

struct ProfileBuilder {
  fz_rect page = fz_empty_rect;
  double page_area = 1.0;
  PdfPageProfile prof;
  bool painted = false;
  std::map<fz_image*, fz_image*> wrappers;  // inner → CountingImage (our ref)
  std::string doc_key;
  int page_1based = 0;

  double fraction(fz_rect r) const {
    r = fz_intersect_rect(r, page);
    if (fz_is_empty_rect(r)) return 0.0;
    return static_cast<double>(r.x1 - r.x0) * static_cast<double>(r.y1 - r.y0) /
           page_area;
  }

  void note_fill(fz_rect bbox, float alpha) {
    if (alpha <= 0.0f) return;
    const double frac = fraction(bbox);
    if (!painted && frac >= kPdfBackgroundFillCoverage) {
      prof.background_fill_ignored = true;
      painted = true;
      return;
    }
    painted = true;
    ++prof.vector_paths;
    prof.vector_coverage += frac;
  }

  void note_stroke(fz_rect bbox, float alpha) {
    if (alpha <= 0.0f) return;
    painted = true;
    ++prof.vector_paths;
    prof.vector_coverage += fraction(bbox);
  }

  void note_shade(fz_rect bbox, float alpha) {
    if (alpha <= 0.0f) return;
    painted = true;
    ++prof.shadings;
    prof.vector_coverage += fraction(bbox);
  }

  static int glyphs(const fz_text* text) {
    int n = 0;
    for (const fz_text_span* span = text ? text->head : nullptr; span; span = span->next) {
      n += span->len;
    }
    return n;
  }

  void note_text(const fz_text* text, float alpha) {
    const int n = glyphs(text);
    if (alpha <= 0.0f) {
      prof.invisible_glyphs += n;
      return;
    }
    painted = true;
    prof.visible_glyphs += n;
  }

  void note_image(fz_image* img, fz_matrix ctm, bool stencil, float alpha) {
    if (alpha <= 0.0f || !img) return;
    painted = true;
    ++prof.image_draws;
    const double len_x = std::hypot(ctm.a, ctm.b);
    const double len_y = std::hypot(ctm.c, ctm.d);
    const double dpi_x = len_x > 1e-6 ? img->w * 72.0 / len_x : 0.0;
    const double dpi_y = len_y > 1e-6 ? img->h * 72.0 / len_y : 0.0;
    const double dpi = std::max(dpi_x, dpi_y);
    const double frac = fraction(fz_transform_rect(fz_unit_rect, ctm));
    prof.image_coverage += frac;
    prof.native_dpi = std::max(prof.native_dpi, dpi);
    if (static_cast<int>(prof.images.size()) < kPdfProfileMaxImages) {
      PdfPageImage info;
      info.width = img->w;
      info.height = img->h;
      info.components = img->n;
      info.bpc = img->bpc;
      info.stencil = stencil;
      info.dpi = dpi;
      info.page_fraction = frac;
      prof.images.push_back(info);
    }
  }

  fz_image* wrap(fz_context* ctx, fz_image* inner) {
    if (!inner) return inner;
    auto it = wrappers.find(inner);
    if (it != wrappers.end()) return it->second;
    fz_image* w = new_counting_image(ctx, inner, doc_key, page_1based);
    wrappers.emplace(inner, w);
    return w;
  }

  void release(fz_context* ctx) {
    for (auto& [inner, w] : wrappers) fz_drop_image(ctx, w);
    wrappers.clear();
  }

  void finish();
};

void ProfileBuilder::finish() {
  prof.width_pt = page.x1 - page.x0;
  prof.height_pt = page.y1 - page.y0;
  prof.image_coverage = std::min(1.0, prof.image_coverage);
  prof.vector_coverage = std::min(1.0, prof.vector_coverage);
  const bool vector =
      prof.vector_paths > 0 || prof.shadings > 0 || prof.visible_glyphs > 0 ||
      prof.clip_glyphs > 0;
  const bool images = prof.image_draws > 0;
  prof.kind = images ? (vector ? PageContentKind::Mixed : PageContentKind::Raster)
                     : (vector ? PageContentKind::Vector : PageContentKind::Empty);

  char buf[512];
  std::string s = page_content_kind_name(prof.kind);
  s += ":";
  if (images) {
    std::snprintf(buf, sizeof buf, " %d image draw%s (sharpest %.0f dpi, %.0f%% of page)",
                  prof.image_draws, prof.image_draws == 1 ? "" : "s", prof.native_dpi,
                  prof.image_coverage * 100.0);
    s += buf;
  }
  if (vector) {
    std::snprintf(buf, sizeof buf, "%s %d path%s, %d shading%s, %d glyph%s",
                  images ? " +" : "", prof.vector_paths,
                  prof.vector_paths == 1 ? "" : "s", prof.shadings,
                  prof.shadings == 1 ? "" : "s", prof.visible_glyphs + prof.clip_glyphs,
                  prof.visible_glyphs + prof.clip_glyphs == 1 ? "" : "s");
    s += buf;
  }
  if (!images && !vector) s += " nothing visible";
  if (prof.invisible_glyphs > 0) {
    std::snprintf(buf, sizeof buf, "; %d invisible glyph%s (OCR layer) ignored",
                  prof.invisible_glyphs, prof.invisible_glyphs == 1 ? "" : "s");
    s += buf;
  }
  if (prof.background_fill_ignored) {
    s += "; page-size background fill ignored";
  }
  if (prof.kind == PageContentKind::Raster && prof.native_dpi > 0.0) {
    const double want = prof.native_dpi / kPdfNativeDpiTolerance;
    int cap = static_cast<int>(
        std::floor(std::log2(static_cast<double>(kPdfLayoutDpi) / want)));
    cap = std::min(cap, 0);
    prof.finest_useful_scale = cap;
    std::snprintf(buf, sizeof buf,
                  " → finest useful scale %d (%.0f dpi ≥ %.0f/%.3g)", cap,
                  static_cast<double>(kPdfLayoutDpi) * std::ldexp(1.0, -cap),
                  prof.native_dpi, kPdfNativeDpiTolerance);
    s += buf;
  } else if (prof.kind == PageContentKind::Mixed || prof.kind == PageContentKind::Vector) {
    s += " → no resolution cap (vector detail at every zoom)";
  }
  prof.summary = std::move(s);
}

struct ProfileDevice {
  fz_device super;
  ProfileBuilder* b;
};

ProfileBuilder* builder(fz_device* dev) {
  return reinterpret_cast<ProfileDevice*>(dev)->b;
}

// Each callback records first (C++ work, no live objects afterwards), then
// forwards as its last statement (which may throw through this frame).

void pd_fill_path(fz_context* ctx, fz_device* dev, const fz_path* path, int even_odd,
                  fz_matrix ctm, fz_colorspace* cs, const float* color, float alpha,
                  fz_color_params cp) {
  builder(dev)->note_fill(fz_bound_path(ctx, path, nullptr, ctm), alpha);
  fz_fill_path(ctx, dev->passthrough, path, even_odd, ctm, cs, color, alpha, cp);
}

void pd_stroke_path(fz_context* ctx, fz_device* dev, const fz_path* path,
                    const fz_stroke_state* stroke, fz_matrix ctm, fz_colorspace* cs,
                    const float* color, float alpha, fz_color_params cp) {
  builder(dev)->note_stroke(fz_bound_path(ctx, path, stroke, ctm), alpha);
  fz_stroke_path(ctx, dev->passthrough, path, stroke, ctm, cs, color, alpha, cp);
}

void pd_fill_text(fz_context* ctx, fz_device* dev, const fz_text* text, fz_matrix ctm,
                  fz_colorspace* cs, const float* color, float alpha,
                  fz_color_params cp) {
  builder(dev)->note_text(text, alpha);
  fz_fill_text(ctx, dev->passthrough, text, ctm, cs, color, alpha, cp);
}

void pd_stroke_text(fz_context* ctx, fz_device* dev, const fz_text* text,
                    const fz_stroke_state* stroke, fz_matrix ctm, fz_colorspace* cs,
                    const float* color, float alpha, fz_color_params cp) {
  builder(dev)->note_text(text, alpha);
  fz_stroke_text(ctx, dev->passthrough, text, stroke, ctm, cs, color, alpha, cp);
}

void pd_clip_text(fz_context* ctx, fz_device* dev, const fz_text* text, fz_matrix ctm,
                  fz_rect scissor) {
  builder(dev)->prof.clip_glyphs += ProfileBuilder::glyphs(text);
  fz_clip_text(ctx, dev->passthrough, text, ctm, scissor);
}

void pd_clip_stroke_text(fz_context* ctx, fz_device* dev, const fz_text* text,
                         const fz_stroke_state* stroke, fz_matrix ctm, fz_rect scissor) {
  builder(dev)->prof.clip_glyphs += ProfileBuilder::glyphs(text);
  fz_clip_stroke_text(ctx, dev->passthrough, text, stroke, ctm, scissor);
}

void pd_ignore_text(fz_context* ctx, fz_device* dev, const fz_text* text, fz_matrix ctm) {
  builder(dev)->prof.invisible_glyphs += ProfileBuilder::glyphs(text);
  fz_ignore_text(ctx, dev->passthrough, text, ctm);
}

void pd_fill_shade(fz_context* ctx, fz_device* dev, fz_shade* shd, fz_matrix ctm,
                   float alpha, fz_color_params cp) {
  builder(dev)->note_shade(fz_bound_shade(ctx, shd, ctm), alpha);
  fz_fill_shade(ctx, dev->passthrough, shd, ctm, alpha, cp);
}

void pd_fill_image(fz_context* ctx, fz_device* dev, fz_image* img, fz_matrix ctm,
                   float alpha, fz_color_params cp) {
  ProfileBuilder* b = builder(dev);
  b->note_image(img, ctm, false, alpha);
  fz_fill_image(ctx, dev->passthrough, b->wrap(ctx, img), ctm, alpha, cp);
}

void pd_fill_image_mask(fz_context* ctx, fz_device* dev, fz_image* img, fz_matrix ctm,
                        fz_colorspace* cs, const float* color, float alpha,
                        fz_color_params cp) {
  ProfileBuilder* b = builder(dev);
  b->note_image(img, ctm, true, alpha);
  fz_fill_image_mask(ctx, dev->passthrough, b->wrap(ctx, img), ctm, cs, color, alpha, cp);
}

void pd_clip_image_mask(fz_context* ctx, fz_device* dev, fz_image* img, fz_matrix ctm,
                        fz_rect scissor) {
  ProfileBuilder* b = builder(dev);
  b->note_image(img, ctm, true, 1.0f);
  fz_clip_image_mask(ctx, dev->passthrough, b->wrap(ctx, img), ctm, scissor);
}

/// Profiling device forwarding to @p target (the passthrough keeps its own
/// reference; the caller still drops @p target).
fz_device* new_profile_device(fz_context* ctx, ProfileBuilder* b, fz_device* target) {
  ProfileDevice* dev = fz_new_derived_passthrough_device(ctx, target, ProfileDevice);
  dev->b = b;
  dev->super.fill_path = pd_fill_path;
  dev->super.stroke_path = pd_stroke_path;
  dev->super.fill_text = pd_fill_text;
  dev->super.stroke_text = pd_stroke_text;
  dev->super.clip_text = pd_clip_text;
  dev->super.clip_stroke_text = pd_clip_stroke_text;
  dev->super.ignore_text = pd_ignore_text;
  dev->super.fill_shade = pd_fill_shade;
  dev->super.fill_image = pd_fill_image;
  dev->super.fill_image_mask = pd_fill_image_mask;
  dev->super.clip_image_mask = pd_clip_image_mask;
  return &dev->super;
}

// --- Document cache -----------------------------------------------------------

struct PageSlot {
  fz_page* page = nullptr;
  fz_rect bounds = fz_empty_rect;
  fz_display_list* list = nullptr;
  std::optional<PdfPageProfile> profile;
  std::string list_error;  // display list build failed (sticky for this slot)
  std::uint64_t last_use = 0;
};

struct DocEntry {
  std::string key;        // cache key (normalized path, "#txt" when forced)
  std::string stats_key;  // normalized path
  std::filesystem::path path;
  std::filesystem::file_time_type mtime{};
  bool as_text = false;

  std::mutex mu;  // guards everything below
  bool open_attempted = false;
  std::string open_error;
  fz_document* doc = nullptr;
  std::map<int, PageSlot> pages;  // 0-based index
  std::uint64_t clock = 0;

  void drop_slot(fz_context* ctx, PageSlot& s) {
    if (s.list) fz_drop_display_list(ctx, s.list);
    if (s.page) fz_drop_page(ctx, s.page);
    s.list = nullptr;
    s.page = nullptr;
  }

  ~DocEntry() {
    fz_context* ctx = thread_ctx();
    if (!ctx) return;
    for (auto& [idx, s] : pages) drop_slot(ctx, s);
    if (doc) fz_drop_document(ctx, doc);
  }
};

struct DocCache {
  std::mutex mu;
  std::list<std::shared_ptr<DocEntry>> lru;  // front = most recent
  std::set<std::string> forced_text;         // paths opened with //text
};

DocCache& doc_cache() {
  static auto* c = new DocCache;
  return *c;
}

std::shared_ptr<DocEntry> cache_entry(const std::filesystem::path& path) {
  std::error_code ec;
  const auto mtime = std::filesystem::last_write_time(path, ec);
  const std::string norm = path.lexically_normal().string();
  auto& c = doc_cache();
  std::lock_guard lock(c.mu);
  if (!g_force_text_key.empty() && g_force_text_key == norm) {
    g_force_text_key.clear();
    c.forced_text.insert(norm);
  }
  const bool as_text = c.forced_text.count(norm) > 0;
  const std::string key = as_text ? norm + "#txt" : norm;
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
  auto e = std::make_shared<DocEntry>();
  e->key = key;
  e->stats_key = norm;
  e->path = path;
  e->mtime = ec ? std::filesystem::file_time_type{} : mtime;
  e->as_text = as_text;
  c.lru.push_front(e);
  while (static_cast<int>(c.lru.size()) > kPdfDocumentCacheSize) {
    c.lru.pop_back();  // users keep their shared_ptr
  }
  return e;
}

void open_locked(fz_context* ctx, DocEntry& e) {
  e.open_attempted = true;
  MarkdownMuPdfLock serial(e.path);
  g_last_mupdf_error.clear();
  fz_document* doc = nullptr;
  fz_var(doc);
  const std::string file = e.path.string();
  if (e.as_text) {
    fz_try(ctx) {
      fz_stream* stm = fz_open_file(ctx, file.c_str());
      fz_try(ctx) { doc = fz_open_document_with_stream(ctx, "txt", stm); }
      fz_always(ctx) { fz_drop_stream(ctx, stm); }
      fz_catch(ctx) { fz_rethrow(ctx); }
    }
    fz_catch(ctx) { doc = nullptr; }
  } else {
    fz_try(ctx) { doc = fz_open_document(ctx, file.c_str()); }
    fz_catch(ctx) { doc = nullptr; }
  }
  e.doc = doc;
  if (!doc) {
    e.open_error = g_last_mupdf_error.empty() ? "cannot open document" : g_last_mupdf_error;
    return;
  }
  with_doc_stats(e.stats_key, [](PdfDocumentRenderStats& d) { ++d.opens; });
}

/// Exclusive access to one cached document for the lifetime of the object.
/// Construct outside fz_try blocks (it has a destructor).
class DocAccess {
 public:
  explicit DocAccess(const std::filesystem::path& path) : ctx_(thread_ctx()) {
    if (!ctx_) {
      error_ = "MuPDF context unavailable";
      return;
    }
    entry_ = cache_entry(path);
    lock_ = std::unique_lock<std::mutex>(entry_->mu, std::try_to_lock);
    if (!lock_.owns_lock()) {
      const Clock::time_point t0 = Clock::now();
      lock_.lock();
      const double waited = ms_since(t0);
      with_doc_stats(entry_->stats_key, [&](PdfDocumentRenderStats& d) {
        ++d.lock_waits;
        d.lock_wait_ms += waited;
        d.lock_wait_max_ms = std::max(d.lock_wait_max_ms, waited);
      });
    }
    if (!entry_->open_attempted) open_locked(ctx_, *entry_);
    if (!entry_->doc) error_ = entry_->open_error;
  }

  explicit operator bool() const { return entry_ && entry_->doc; }
  fz_context* ctx() const { return ctx_; }
  fz_document* doc() const { return entry_ ? entry_->doc : nullptr; }
  DocEntry& entry() const { return *entry_; }
  std::shared_ptr<DocEntry> share() const { return entry_; }
  const std::string& error() const { return error_; }

  /// Loaded page slot (page object + bounds), or nullptr with error().
  PageSlot* slot(int page_1based);

  fz_page* page(int page_1based) {
    PageSlot* s = slot(page_1based);
    return s ? s->page : nullptr;
  }

  /// Slot with its display list and profile built, or nullptr with error().
  PageSlot* listed_slot(int page_1based);

 private:
  void trim_pages();

  fz_context* ctx_ = nullptr;
  std::shared_ptr<DocEntry> entry_;
  std::unique_lock<std::mutex> lock_;
  std::string error_;
};

PageSlot* DocAccess::slot(int page_1based) {
  if (!*this) return nullptr;
  if (page_1based < 1) {
    error_ = "page numbers start at 1";
    return nullptr;
  }
  DocEntry& e = *entry_;
  const int idx = page_1based - 1;
  auto it = e.pages.find(idx);
  if (it != e.pages.end() && it->second.page) {
    it->second.last_use = ++e.clock;
    return &it->second;
  }
  MarkdownMuPdfLock serial(e.path);
  fz_context* ctx = ctx_;
  fz_document* doc = e.doc;
  int n = 0;
  fz_page* page = nullptr;
  fz_rect bounds = fz_empty_rect;
  fz_var(page);
  fz_var(bounds);
  fz_var(n);
  g_last_mupdf_error.clear();
  fz_try(ctx) {
    n = fz_count_pages(ctx, doc);
    if (idx < n) {
      page = fz_load_page(ctx, doc, idx);
      bounds = fz_bound_page(ctx, page);
    }
  }
  fz_catch(ctx) {
    if (page) fz_drop_page(ctx, page);
    page = nullptr;
  }
  if (!page) {
    if (idx >= n && n > 0) {
      error_ = "page " + std::to_string(page_1based) + " out of range (document has " +
               std::to_string(n) + ")";
    } else {
      error_ = g_last_mupdf_error.empty() ? "cannot load page" : g_last_mupdf_error;
    }
    return nullptr;
  }
  PageSlot& s = e.pages[idx];
  s.page = page;
  s.bounds = bounds;
  s.last_use = ++e.clock;
  trim_pages();
  return &e.pages[idx];
}

void DocAccess::trim_pages() {
  DocEntry& e = *entry_;
  while (static_cast<int>(e.pages.size()) > kPdfPageCacheSize) {
    auto victim = e.pages.begin();
    for (auto it = e.pages.begin(); it != e.pages.end(); ++it) {
      if (it->second.last_use < victim->second.last_use) victim = it;
    }
    e.drop_slot(ctx_, victim->second);
    e.pages.erase(victim);
  }
}

PageSlot* DocAccess::listed_slot(int page_1based) {
  PageSlot* s = slot(page_1based);
  if (!s) return nullptr;
  if (s->list) return s;
  if (!s->list_error.empty()) {
    error_ = s->list_error;
    return nullptr;
  }
  fz_context* ctx = ctx_;
  ProfileBuilder b;
  b.page = s->bounds;
  b.page_area = std::max(1.0, static_cast<double>(s->bounds.x1 - s->bounds.x0) *
                                  static_cast<double>(s->bounds.y1 - s->bounds.y0));
  b.doc_key = entry_->stats_key;
  b.page_1based = page_1based;
  fz_display_list* list = nullptr;
  fz_device* list_dev = nullptr;
  fz_device* dev = nullptr;
  fz_var(list);
  fz_var(list_dev);
  fz_var(dev);
  int ok = 0;
  fz_var(ok);
  g_last_mupdf_error.clear();
  const Clock::time_point t0 = Clock::now();
  fz_try(ctx) {
    list = fz_new_display_list(ctx, s->bounds);
    list_dev = fz_new_list_device(ctx, list);
    dev = new_profile_device(ctx, &b, list_dev);
    fz_run_page(ctx, s->page, dev, fz_identity, nullptr);
    fz_close_device(ctx, dev);  // closes list_dev too
    ok = 1;
  }
  fz_always(ctx) {
    fz_drop_device(ctx, dev);
    fz_drop_device(ctx, list_dev);
  }
  fz_catch(ctx) {
    fz_drop_display_list(ctx, list);
    list = nullptr;
    ok = 0;
  }
  const double ms = ms_since(t0);
  b.release(ctx);  // the list holds its own references to the wrappers
  if (!ok) {
    s->list_error = "display list: " +
                    (g_last_mupdf_error.empty() ? std::string("page could not be run")
                                                : g_last_mupdf_error);
    error_ = s->list_error;
    with_page_stats(entry_->stats_key, page_1based,
                    [&](PdfDocumentRenderStats&, PdfPageRenderStats& p) {
                      p.last_error = s->list_error;
                    });
    return nullptr;
  }
  b.finish();
  s->list = list;
  s->profile = b.prof;
  with_page_stats(entry_->stats_key, page_1based,
                  [&](PdfDocumentRenderStats&, PdfPageRenderStats& p) {
                    ++p.display_list_builds;
                    p.display_list_ms += ms;
                    p.profile = b.prof;
                  });
  return s;
}

/// Display list + what is needed to run it without the document lock.
struct ListLease {
  std::shared_ptr<DocEntry> entry;  // keeps the document (Type3 glyphs) alive
  fz_display_list* list = nullptr;  // own reference
  fz_rect bounds = fz_empty_rect;
  PdfPageProfile profile;
  std::string error;

  ListLease() = default;
  ListLease(const ListLease&) = delete;
  ListLease& operator=(const ListLease&) = delete;
  ~ListLease() {
    if (list) fz_drop_display_list(thread_ctx(), list);
  }
};

void lease_list(const std::filesystem::path& path, int page_1based, ListLease& out) {
  DocAccess acc(path);
  PageSlot* s = acc ? acc.listed_slot(page_1based) : nullptr;
  if (!s) {
    out.error = acc.error().empty() ? "cannot open document" : acc.error();
    return;
  }
  out.entry = acc.share();
  out.list = fz_keep_display_list(acc.ctx(), s->list);
  out.bounds = s->bounds;
  out.profile = *s->profile;
}

std::optional<PdfRaster> pixmap_to_rgb(fz_context* ctx, fz_pixmap* pix) {
  if (!ctx || !pix) return std::nullopt;
  const int w = fz_pixmap_width(ctx, pix);
  const int h = fz_pixmap_height(ctx, pix);
  const int n = fz_pixmap_components(ctx, pix);
  // n==1/2 grey(+alpha), n==3/4 rgb(+alpha); reject empty or exotic.
  if (w <= 0 || h <= 0 || n < 1 || n > 4) return std::nullopt;

  PdfRaster out;
  out.width = w;
  out.height = h;
  out.rgb.resize(static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 3u);
  const unsigned char* src = fz_pixmap_samples(ctx, pix);
  const int stride = fz_pixmap_stride(ctx, pix);
  for (int y = 0; y < h; ++y) {
    const unsigned char* row = src + static_cast<std::size_t>(y) * stride;
    std::uint8_t* dst =
        out.rgb.data() + static_cast<std::size_t>(y) * static_cast<std::size_t>(w) * 3u;
    if (n == 3 || n == 4) {
      for (int x = 0; x < w; ++x) {
        dst[x * 3 + 0] = row[x * n + 0];
        dst[x * 3 + 1] = row[x * n + 1];
        dst[x * 3 + 2] = row[x * n + 2];
      }
    } else {  // n == 1 || n == 2
      for (int x = 0; x < w; ++x) {
        const unsigned char g = row[x * n];
        dst[x * 3 + 0] = g;
        dst[x * 3 + 1] = g;
        dst[x * 3 + 2] = g;
      }
    }
  }
  return out;
}

/// Run @p list into a white RGB pixmap covering device rect @p bbox under
/// @p ctm. Thread-safe (no document access). Error text in @p error.
std::optional<PdfRaster> run_list_region(fz_context* ctx, fz_display_list* list,
                                         fz_matrix ctm, fz_irect bbox,
                                         std::string* error) {
  fz_rect clip = fz_rect_from_irect(bbox);
  fz_var(ctm);
  fz_var(clip);
  fz_var(bbox);
  fz_pixmap* pix = nullptr;
  fz_device* dev = nullptr;
  fz_var(pix);
  fz_var(dev);
  int failed = 0;
  fz_var(failed);
  g_last_mupdf_error.clear();
  fz_try(ctx) {
    pix = fz_new_pixmap_with_bbox(ctx, fz_device_rgb(ctx), bbox, nullptr, 0);
    fz_clear_pixmap_with_value(ctx, pix, 0xff);
    dev = fz_new_draw_device(ctx, fz_identity, pix);
    fz_run_display_list(ctx, list, dev, ctm, clip, nullptr);
    fz_close_device(ctx, dev);
  }
  fz_always(ctx) {
    fz_drop_device(ctx, dev);
    dev = nullptr;
  }
  fz_catch(ctx) {
    fz_drop_pixmap(ctx, pix);
    pix = nullptr;
    failed = 1;
  }
  if (failed || !pix) {
    if (error) {
      *error = "render: " + (g_last_mupdf_error.empty() ? std::string("MuPDF draw failed")
                                                        : g_last_mupdf_error);
    }
    return std::nullopt;
  }
  auto out = pixmap_to_rgb(ctx, pix);
  fz_drop_pixmap(ctx, pix);
  if (!out && error) *error = "render: unsupported pixmap format";
  return out;
}

std::optional<std::string> refusal_for(const PdfPageProfile& prof, int scale) {
  if (!prof.finest_useful_scale || scale >= *prof.finest_useful_scale) {
    return std::nullopt;
  }
  char buf[256];
  std::snprintf(buf, sizeof buf,
                "scale %d (%.0f dpi) is finer than this raster page needs: sharpest "
                "image %.0f dpi, finest useful scale %d",
                scale, pdf_dpi_for_scale(scale), prof.native_dpi,
                *prof.finest_useful_scale);
  return std::string(buf);
}

}  // namespace

void mupdf_force_next_open_as_text(const std::filesystem::path& path)
{
  g_force_text_key = path.lexically_normal().string();
}

std::string mupdf_last_error()
{
  return g_last_mupdf_error;
}

void mupdf_clear_last_error()
{
  g_last_mupdf_error.clear();
}

std::optional<int> mupdf_page_count(const std::filesystem::path& path) {
  DocAccess acc(path);
  if (!acc) return std::nullopt;
  fz_context* ctx = acc.ctx();
  fz_document* doc = acc.doc();
  int n = 0;
  fz_var(n);
  fz_try(ctx) { n = fz_count_pages(ctx, doc); }
  fz_catch(ctx) { n = 0; }
  if (n <= 0) return std::nullopt;
  return n;
}

std::optional<Size> mupdf_page_size_72dpi(const std::filesystem::path& path,
                                          int page_1based) {
  DocAccess acc(path);
  PageSlot* s = acc.slot(page_1based);
  if (!s) return std::nullopt;
  const fz_rect box = s->bounds;
  // Integer page-space points (legacy / reporting). Prefer
  // mupdf_page_layout_size for the device pixel grid.
  const int w = std::max(1, static_cast<int>(std::lround(box.x1 - box.x0)));
  const int h = std::max(1, static_cast<int>(std::lround(box.y1 - box.y0)));
  return Size{w, h};
}

namespace {

/// Layout pixels at kPdfLayoutDpi: one lround from continuous page bounds.
/// lround(lround(pt) * dpi/72) can differ by 1px from lround(pt * dpi/72).
Size layout_size_from_bounds(fz_rect box) {
  const double sx = static_cast<double>(kPdfLayoutDpi) / 72.0;
  const int w = std::max(1, static_cast<int>(std::lround((box.x1 - box.x0) * sx)));
  const int h = std::max(1, static_cast<int>(std::lround((box.y1 - box.y0) * sx)));
  return Size{w, h};
}

}  // namespace

std::optional<Size> mupdf_page_layout_size(const std::filesystem::path& path,
                                           int page_1based) {
  DocAccess acc(path);
  PageSlot* s = acc.slot(page_1based);
  if (!s) return std::nullopt;
  return layout_size_from_bounds(s->bounds);
}

std::optional<PdfPageProfile> mupdf_page_profile(const std::filesystem::path& path,
                                                 int page_1based, std::string* error) {
  DocAccess acc(path);
  PageSlot* s = acc ? acc.listed_slot(page_1based) : nullptr;
  if (!s) {
    if (error) *error = acc.error().empty() ? "cannot open document" : acc.error();
    return std::nullopt;
  }
  return *s->profile;
}

std::optional<std::string> mupdf_scale_refusal(const std::filesystem::path& path,
                                               int page_1based, int scale) {
  std::string error;
  auto prof = mupdf_page_profile(path, page_1based, &error);
  if (!prof) return std::nullopt;  // the render reports the real failure
  return refusal_for(*prof, scale);
}

std::optional<PdfRaster> mupdf_rasterize_page(const std::filesystem::path& path,
                                              int page_1based, int max_edge) {
  if (max_edge < 1) return std::nullopt;
  ListLease lease;
  lease_list(path, page_1based, lease);
  if (!lease.list) return std::nullopt;
  fz_context* ctx = thread_ctx();
  const fz_rect box = lease.bounds;
  const double pw = std::max(1.0, static_cast<double>(box.x1 - box.x0));
  const double ph = std::max(1.0, static_cast<double>(box.y1 - box.y0));
  const double scale = static_cast<double>(max_edge) / std::max(pw, ph);
  const fz_matrix ctm = fz_scale(static_cast<float>(scale), static_cast<float>(scale));
  const fz_irect bbox = fz_round_rect(fz_transform_rect(box, ctm));
  std::string error;
  auto out = run_list_region(ctx, lease.list, ctm, bbox, &error);
  with_page_stats(lease.entry->stats_key, page_1based,
                  [&](PdfDocumentRenderStats&, PdfPageRenderStats& p) {
                    ++p.page_rasters;
                    if (!out) p.last_error = error;
                  });
  return out;
}

std::optional<PdfRaster> mupdf_rasterize_page_region(
    const std::filesystem::path& path, int page_1based, double dpi, int px,
    int py, int pw, int ph) {
  if (page_1based < 1 || pw <= 0 || ph <= 0 || dpi <= 0.0) return std::nullopt;
  ListLease lease;
  lease_list(path, page_1based, lease);
  if (!lease.list) return std::nullopt;
  // Unit chain: page points (fz_bound_page, origin at the page box corner)
  // → device pixels via scale dpi/72; (px,py,pw,ph) is the exclusive device
  // rect. Layout size is one lround(page_pt * dpi/72) so this grid matches.
  const float s = static_cast<float>(dpi / 72.0);
  const fz_matrix ctm = fz_pre_translate(fz_scale(s, s), -lease.bounds.x0, -lease.bounds.y0);
  fz_irect bbox;
  bbox.x0 = px;
  bbox.y0 = py;
  bbox.x1 = px + pw;
  bbox.y1 = py + ph;
  return run_list_region(thread_ctx(), lease.list, ctm, bbox, nullptr);
}

PdfCellRender mupdf_render_tile_cell(const std::filesystem::path& path,
                                     int page_1based, int scale, int x, int y) {
  PdfCellRender out;
  if (x < 0 || y < 0) {
    out.status = TileStatus::Unavailable;
    out.error = "negative cell index";
    return out;
  }
  ListLease lease;
  lease_list(path, page_1based, lease);
  if (!lease.list) {
    out.status = TileStatus::Failed;
    out.error = lease.error;
    return out;
  }
  const std::string& key = lease.entry->stats_key;
  if (auto why = refusal_for(lease.profile, scale)) {
    with_page_stats(key, page_1based, [](PdfDocumentRenderStats&, PdfPageRenderStats& p) {
      ++p.cells_refused;
    });
    out.status = TileStatus::Unavailable;
    out.error = *why;
    return out;
  }
  const Size layout = layout_size_from_bounds(lease.bounds);
  const Size full = pdf_page_size_at_scale(layout, scale);
  int left = 0, top = 0, tw = 0, th = 0;
  tile_cell_pixel_rect(full.width, full.height, x, y, &left, &top, &tw, &th);
  if (tw <= 0 || th <= 0) {
    out.status = TileStatus::Unavailable;
    char buf[128];
    std::snprintf(buf, sizeof buf, "cell %d,%d outside the %dx%d page at scale %d", x, y,
                  full.width, full.height, scale);
    out.error = buf;
    return out;
  }
  // Same unit chain as mupdf_rasterize_page_region: page points → device
  // pixels at dpi/72; the cell is an exclusive rect of that grid.
  const float sc = static_cast<float>(pdf_dpi_for_scale(scale) / 72.0);
  const fz_matrix ctm =
      fz_pre_translate(fz_scale(sc, sc), -lease.bounds.x0, -lease.bounds.y0);
  fz_irect bbox;
  bbox.x0 = left;
  bbox.y0 = top;
  bbox.x1 = left + tw;
  bbox.y1 = top + th;
  const Clock::time_point t0 = Clock::now();
  std::string error;
  auto raster = run_list_region(thread_ctx(), lease.list, ctm, bbox, &error);
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
    out.status = TileStatus::Failed;
    out.error = error;
    return out;
  }
  out.status = TileStatus::Ok;
  out.raster = std::move(raster);
  return out;
}

std::optional<PdfDocumentRenderStats> mupdf_document_render_stats(
    const std::filesystem::path& path) {
  const std::string key = path.lexically_normal().string();
  auto& r = stats_registry();
  std::lock_guard lock(r.mu);
  auto it = r.docs.find(key);
  if (it == r.docs.end()) return std::nullopt;
  return it->second;
}

void pdf_set_full_image_decode_budget(std::size_t bytes) {
  g_full_decode_budget.store(bytes, std::memory_order_relaxed);
}

std::size_t pdf_full_image_decode_budget() {
  return g_full_decode_budget.load(std::memory_order_relaxed);
}

void pdf_release_document_cache() {
  std::list<std::shared_ptr<DocEntry>> drop;
  {
    auto& c = doc_cache();
    std::lock_guard lock(c.mu);
    drop.swap(c.lru);
  }
  drop.clear();  // DocEntry destructors run outside the cache lock
  if (fz_context* ctx = thread_ctx()) fz_empty_store(ctx);
}

void mupdf_reset_render_stats() {
  auto& r = stats_registry();
  std::lock_guard lock(r.mu);
  r.docs.clear();
}

#if defined(THUMTOO_HAVE_MUPDF)
/// True for a usable embedded raster Image XObject (not a stencil ImageMask).
[[nodiscard]] bool mupdf_obj_is_raster_image(fz_context* ctx, pdf_obj* obj) {
  if (!ctx || !obj) return false;
  obj = pdf_resolve_indirect(ctx, obj);
  if (!pdf_name_eq(ctx, pdf_dict_get(ctx, obj, PDF_NAME(Subtype)), PDF_NAME(Image))) {
    return false;
  }
  // Stencil / soft-mask bitmaps are Subtype Image with ImageMask true — skip.
  if (pdf_dict_get_bool(ctx, obj, PDF_NAME(ImageMask))) {
    return false;
  }
  const int w = pdf_to_int(ctx, pdf_dict_get(ctx, obj, PDF_NAME(Width)));
  const int h = pdf_to_int(ctx, pdf_dict_get(ctx, obj, PDF_NAME(Height)));
  // Reject empty / broken dicts (mutool extract still lists them; we need pixels).
  return w > 0 && h > 0;
}

/// mutool-style: walk the whole xref for Image objects (stable object-number order).
/// Page-resource walks miss images only referenced from deeper forms / content and
/// can disagree with load. Returns 1-based index → object number, or 0.
[[nodiscard]] int mupdf_find_embedded_image_objnum(fz_context* ctx, pdf_document* pdf,
                                                   fz_document* /*doc*/, int image_1based) {
  if (!ctx || !pdf || image_1based < 1) return 0;
  int found_num = 0;
  int seen = 0;
  int xref_len = 0;
  int num = 0;
  fz_var(found_num);
  fz_var(seen);
  fz_var(xref_len);
  fz_var(num);
  fz_try(ctx) { xref_len = pdf_xref_len(ctx, pdf); }
  fz_catch(ctx) { return 0; }

  for (num = 1; num < xref_len && found_num == 0; ++num) {
    pdf_obj* obj = nullptr;
    fz_var(obj);
    fz_try(ctx) {
      obj = pdf_load_object(ctx, pdf, num);
      if (mupdf_obj_is_raster_image(ctx, obj)) {
        ++seen;
        if (seen == image_1based) {
          found_num = num;
        }
      }
    }
    fz_always(ctx) {
      if (obj) pdf_drop_obj(ctx, obj);
    }
    fz_catch(ctx) { /* skip broken object */ }
  }
  return found_num;
}

#endif  // THUMTOO_HAVE_MUPDF

std::optional<int> mupdf_embedded_image_count(const std::filesystem::path& path) {
#if !defined(THUMTOO_HAVE_MUPDF)
  (void)path;
  return std::nullopt;
#else
  DocAccess acc(path);
  if (!acc) return std::nullopt;
  fz_context* ctx = acc.ctx();
  fz_document* doc = acc.doc();
  pdf_document* pdf = pdf_document_from_fz_document(ctx, doc);
  if (!pdf) return std::nullopt;

  int total = 0;
  int xref_len = 0;
  int num = 0;
  fz_var(total);
  fz_var(xref_len);
  fz_var(num);
  fz_try(ctx) { xref_len = pdf_xref_len(ctx, pdf); }
  fz_catch(ctx) { return std::nullopt; }

  for (num = 1; num < xref_len; ++num) {
    pdf_obj* obj = nullptr;
    fz_var(obj);
    fz_try(ctx) {
      obj = pdf_load_object(ctx, pdf, num);
      if (mupdf_obj_is_raster_image(ctx, obj)) {
        ++total;
      }
    }
    fz_always(ctx) {
      if (obj) pdf_drop_obj(ctx, obj);
    }
    fz_catch(ctx) { /* skip */ }
  }
  return total;
#endif
}

std::optional<PdfRaster> mupdf_rasterize_embedded_image(
    const std::filesystem::path& path, int image_1based, int max_edge) {
#if !defined(THUMTOO_HAVE_MUPDF)
  (void)path;
  (void)image_1based;
  (void)max_edge;
  return std::nullopt;
#else
  if (image_1based < 1) return std::nullopt;
  DocAccess acc(path);
  if (!acc) return std::nullopt;
  fz_context* ctx = acc.ctx();
  fz_document* doc = acc.doc();
  pdf_document* pdf = pdf_document_from_fz_document(ctx, doc);
  if (!pdf) return std::nullopt;

  const int objnum = mupdf_find_embedded_image_objnum(ctx, pdf, doc, image_1based);
  if (objnum <= 0) return std::nullopt;

  pdf_obj* target = nullptr;
  fz_image* image = nullptr;
  fz_pixmap* pix = nullptr;
  fz_var(target);
  fz_var(image);
  fz_var(pix);
  std::optional<PdfRaster> out;
  fz_try(ctx) {
    // mutool extract uses an indirect ref; load_object alone is not enough for
    // all stream Image XObjects across MuPDF versions.
    target = pdf_new_indirect(ctx, pdf, objnum, 0);
    if (!target) {
      fz_throw(ctx, FZ_ERROR_GENERIC, "pdfimage: missing object %d", objnum);
    }
    image = pdf_load_image(ctx, pdf, target);
    // Native resolution pixmap (identity matrix / full image).
    pix = fz_get_pixmap_from_image(ctx, image, nullptr, nullptr, nullptr, nullptr);
    if (pix) {
      // Convert CMYK/etc.; leave DeviceGray and DeviceRGB as-is for pixmap_to_rgb.
      fz_colorspace* cs = fz_pixmap_colorspace(ctx, pix);
      if (cs && fz_colorspace_n(ctx, cs) != 3 && fz_colorspace_n(ctx, cs) != 1) {
        fz_pixmap* rgb = fz_convert_pixmap(ctx, pix, fz_device_rgb(ctx), nullptr,
                                           nullptr, fz_default_color_params, 0);
        fz_drop_pixmap(ctx, pix);
        pix = rgb;
      }
      if (max_edge > 0 && pix) {
        const int w = fz_pixmap_width(ctx, pix);
        const int h = fz_pixmap_height(ctx, pix);
        const int long_edge = std::max(w, h);
        if (long_edge > max_edge) {
          const float scale =
              static_cast<float>(max_edge) / static_cast<float>(long_edge);
          const int nw = std::max(1, static_cast<int>(std::lround(w * scale)));
          const int nh = std::max(1, static_cast<int>(std::lround(h * scale)));
          fz_pixmap* scaled = fz_new_pixmap(ctx, fz_pixmap_colorspace(ctx, pix),
                                            nw, nh, nullptr, 0);
          if (scaled) {
            fz_clear_pixmap_with_value(ctx, scaled, 0);
            const int n = fz_pixmap_components(ctx, pix);
            const unsigned char* src = fz_pixmap_samples(ctx, pix);
            unsigned char* dst = fz_pixmap_samples(ctx, scaled);
            for (int y = 0; y < nh; ++y) {
              const int sy = std::min(h - 1, static_cast<int>(y / scale));
              for (int x = 0; x < nw; ++x) {
                const int sx = std::min(w - 1, static_cast<int>(x / scale));
                const unsigned char* s =
                    src + (static_cast<size_t>(sy) * w + sx) * n;
                unsigned char* d =
                    dst + (static_cast<size_t>(y) * nw + x) * n;
                for (int c = 0; c < n; ++c) d[c] = s[c];
              }
            }
            fz_drop_pixmap(ctx, pix);
            pix = scaled;
          }
        }
      }
      out = pixmap_to_rgb(ctx, pix);
    }
  }
  fz_always(ctx) {
    if (pix) fz_drop_pixmap(ctx, pix);
    if (image) fz_drop_image(ctx, image);
    if (target) pdf_drop_obj(ctx, target);
  }
  fz_catch(ctx) { out = std::nullopt; }
  return out;
#endif
}

std::optional<Size> mupdf_embedded_image_size(const std::filesystem::path& path,
                                              int image_1based) {
#if !defined(THUMTOO_HAVE_MUPDF)
  (void)path;
  (void)image_1based;
  return std::nullopt;
#else
  if (image_1based < 1) return std::nullopt;
  DocAccess acc(path);
  if (!acc) return std::nullopt;
  fz_context* ctx = acc.ctx();
  fz_document* doc = acc.doc();
  pdf_document* pdf = pdf_document_from_fz_document(ctx, doc);
  if (!pdf) return std::nullopt;

  const int objnum = mupdf_find_embedded_image_objnum(ctx, pdf, doc, image_1based);
  if (objnum <= 0) return std::nullopt;

  pdf_obj* target = nullptr;
  std::optional<Size> out;
  fz_var(target);
  fz_try(ctx) {
    target = pdf_new_indirect(ctx, pdf, objnum, 0);
    if (!target) {
      fz_throw(ctx, FZ_ERROR_GENERIC, "pdfimage: missing object %d", objnum);
    }
    pdf_obj* resolved = pdf_resolve_indirect(ctx, target);
    // Prefer dictionary /Width /Height (no stream decode). Fallback to fz_image.
    const int w = pdf_to_int(ctx, pdf_dict_get(ctx, resolved, PDF_NAME(Width)));
    const int h = pdf_to_int(ctx, pdf_dict_get(ctx, resolved, PDF_NAME(Height)));
    if (w > 0 && h > 0) {
      out = Size{w, h};
    } else {
      fz_image* image = pdf_load_image(ctx, pdf, target);
      if (image) {
        out = Size{image->w, image->h};
        fz_drop_image(ctx, image);
      }
    }
  }
  fz_always(ctx) {
    if (target) pdf_drop_obj(ctx, target);
  }
  fz_catch(ctx) { out = std::nullopt; }
  return out;
#endif
}



std::optional<PdfRaster> mupdf_page_thumb_rgb(const std::filesystem::path& path,
                                              int page_1based) {
#if !defined(THUMTOO_HAVE_MUPDF)
  (void)path;
  (void)page_1based;
  return std::nullopt;
#else
  if (page_1based < 1) return std::nullopt;
  DocAccess acc(path);
  if (!acc) return std::nullopt;
  fz_context* ctx = acc.ctx();
  fz_document* doc = acc.doc();
  pdf_document* pdf = pdf_document_from_fz_document(ctx, doc);
  if (!pdf) return std::nullopt;

  // Page /Thumb is an optional Image stream on the page dictionary — not a
  // full page render. Missing on many PDFs; cheap when present.
  pdf_obj* page_obj = nullptr;
  pdf_obj* thumb = nullptr;
  fz_image* image = nullptr;
  fz_pixmap* pix = nullptr;
  fz_var(page_obj);
  fz_var(thumb);
  fz_var(image);
  fz_var(pix);
  std::optional<PdfRaster> out;
  fz_try(ctx) {
    page_obj = pdf_lookup_page_obj(ctx, pdf, page_1based - 1);
    if (!page_obj) {
      fz_throw(ctx, FZ_ERROR_GENERIC, "pdf thumb: no page obj");
    }
    thumb = pdf_dict_get(ctx, page_obj, PDF_NAME(Thumb));
    if (!thumb || pdf_is_null(ctx, thumb)) {
      fz_throw(ctx, FZ_ERROR_GENERIC, "pdf thumb: no /Thumb");
    }
    // Keep a strong ref — dict_get does not always own for load_image.
    thumb = pdf_keep_obj(ctx, thumb);
    if (!mupdf_obj_is_raster_image(ctx, thumb)) {
      fz_throw(ctx, FZ_ERROR_GENERIC, "pdf thumb: not a raster image");
    }
    image = pdf_load_image(ctx, pdf, thumb);
    pix = fz_get_pixmap_from_image(ctx, image, nullptr, nullptr, nullptr, nullptr);
    if (!pix) {
      fz_throw(ctx, FZ_ERROR_GENERIC, "pdf thumb: empty pixmap");
    }
    fz_colorspace* cs = fz_pixmap_colorspace(ctx, pix);
    if (cs && fz_colorspace_n(ctx, cs) != 3 && fz_colorspace_n(ctx, cs) != 1) {
      fz_pixmap* rgb = fz_convert_pixmap(ctx, pix, fz_device_rgb(ctx), nullptr,
                                         nullptr, fz_default_color_params, 0);
      fz_drop_pixmap(ctx, pix);
      pix = rgb;
    }
    out = pixmap_to_rgb(ctx, pix);
  }
  fz_always(ctx) {
    if (pix) fz_drop_pixmap(ctx, pix);
    if (image) fz_drop_image(ctx, image);
    if (thumb) pdf_drop_obj(ctx, thumb);
  }
  fz_catch(ctx) { out = std::nullopt; }
  return out;
#endif
}

[[nodiscard]] bool is_external_link_uri(const char* uri) {
  if (!uri || !uri[0]) return true;
  auto starts_ci = [](const char* s, const char* prefix) {
    for (; *prefix; ++prefix, ++s) {
      if (!*s) return false;
      const char a = (*s >= 'A' && *s <= 'Z') ? static_cast<char>(*s - 'A' + 'a') : *s;
      const char b = (*prefix >= 'A' && *prefix <= 'Z')
                         ? static_cast<char>(*prefix - 'A' + 'a')
                         : *prefix;
      if (a != b) return false;
    }
    return true;
  };
  return starts_ci(uri, "http:") || starts_ci(uri, "https:")
         || starts_ci(uri, "mailto:") || starts_ci(uri, "ftp:")
         || starts_ci(uri, "file:");
}

/** Resolve internal PDF/EPUB link (#dest or relative path) to 0-based page. */
[[nodiscard]] bool resolve_internal_link_page(fz_context* ctx, fz_document* doc,
                                              const char* uri, int* page_0based,
                                              float* x_out, float* y_out) {
  if (!ctx || !doc || !uri || !uri[0]) return false;
  if (is_external_link_uri(uri)) return false;
  int page = -1;
  float lx = 0, ly = 0;
  int ok = 0;
  fz_var(page);
  fz_var(lx);
  fz_var(ly);
  fz_var(ok);
  fz_try(ctx) {
    fz_location loc = fz_resolve_link(ctx, doc, uri, &lx, &ly);
    page = loc.page;
    ok = 1;
  }
  fz_catch(ctx) { ok = 0; }
  if (!ok || page < 0) return false;
  if (page_0based) *page_0based = page;
  if (x_out) *x_out = lx;
  if (y_out) *y_out = ly;
  return true;
}

// Back-compat name used by older call sites in this file.
[[nodiscard]] bool resolve_hash_link_page(fz_context* ctx, fz_document* doc,
                                          const char* uri, int* page_0based,
                                          float* x_out, float* y_out) {
  return resolve_internal_link_page(ctx, doc, uri, page_0based, x_out, y_out);
}

std::optional<PageTextLayer> mupdf_page_text_layer(const std::filesystem::path& path,
                                                   int page_1based) {
#if !defined(THUMTOO_HAVE_MUPDF)
  (void)path;
  (void)page_1based;
  return std::nullopt;
#else
  if (page_1based < 1) return std::nullopt;
  DocAccess acc(path);
  fz_page* page = acc.page(page_1based);
  if (!page) return std::nullopt;
  fz_context* ctx = acc.ctx();

  PageTextLayer layer;
  layer.page_1based = page_1based;

  fz_rect box = fz_empty_rect;
  fz_var(box);
  int ok = 0;
  fz_var(ok);
  fz_try(ctx) {
    box = fz_bound_page(ctx, page);
    ok = 1;
  }
  fz_catch(ctx) { ok = 0; }
  if (!ok) return std::nullopt;
  layer.page_bounds = TextRect{box.x0, box.y0, box.x1, box.y1};
  // MuPDF page/stext space: origin top-left, Y down (not PDF user space).
  // See https://mupdf.readthedocs.io/en/latest/coordinate-system.html
  layer.page_y_up = false;

  // Text: one structured-text pass → line-level regions (good for search/select).
  fz_stext_options opts{};
  opts.flags = 0;
  fz_stext_page* stext = nullptr;
  fz_var(stext);
  fz_try(ctx) { stext = fz_new_stext_page_from_page(ctx, page, &opts); }
  fz_catch(ctx) { stext = nullptr; }
  if (stext) {
    int block_index = 0;
    for (fz_stext_block* block = stext->first_block; block; block = block->next) {
      if (block->type != FZ_STEXT_BLOCK_TEXT) continue;
      const int this_block = block_index++;
      for (fz_stext_line* line = block->u.t.first_line; line; line = line->next) {
        std::string line_text;
        line_text.reserve(64);
        for (fz_stext_char* ch = line->first_char; ch; ch = ch->next) {
          if (ch->c == 0) continue;
          utf8_append_codepoint(line_text, ch->c);
        }
        // Trim trailing whitespace-only lines.
        while (!line_text.empty() &&
               (line_text.back() == ' ' || line_text.back() == '\t' ||
                line_text.back() == '\r' || line_text.back() == '\n')) {
          line_text.pop_back();
        }
        if (line_text.empty()) continue;

        TextRegion reg;
        reg.role = TextRegionRole::Text;
        reg.block_id = this_block;
        reg.text = std::move(line_text);
        reg.bbox = TextRect{line->bbox.x0, line->bbox.y0, line->bbox.x1, line->bbox.y1};
        if (!reg.bbox.empty()) layer.regions.push_back(std::move(reg));
      }
    }
    fz_drop_stext_page(ctx, stext);
  }

  // Links: separate pass. Snapshot first; resolve with POD-only helper.
  fz_link* links = nullptr;
  fz_var(links);
  fz_try(ctx) { links = fz_load_links(ctx, page); }
  fz_catch(ctx) { links = nullptr; }
  struct LinkSnap {
    TextRect bbox;
    std::string uri;
  };
  std::vector<LinkSnap> snaps;
  for (fz_link* link = links; link; link = link->next) {
    LinkSnap s;
    s.bbox = TextRect{link->rect.x0, link->rect.y0, link->rect.x1, link->rect.y1};
    if (s.bbox.empty()) continue;
    if (link->uri) s.uri = link->uri;
    snaps.push_back(std::move(s));
  }
  if (links) {
    fz_drop_link(ctx, links);
    links = nullptr;
  }
  fz_document* doc = acc.doc();
  for (std::size_t i = 0; i < snaps.size(); ++i) {
    // Copy fields before any fz_try so no reference lives across longjmp.
    const TextRect bbox = snaps[i].bbox;
    const std::string uri = snaps[i].uri;
    TextRegion reg;
    reg.role = TextRegionRole::Link;
    reg.bbox = bbox;
    if (!uri.empty()) {
      int dest_page = -1;
      float lx = 0, ly = 0;
      if (resolve_internal_link_page(ctx, doc, uri.c_str(), &dest_page, &lx, &ly)) {
        reg.target.kind = TextLinkTargetKind::InternalPage;
        reg.target.page_1based = dest_page + 1;
        reg.target.x = static_cast<double>(lx);
        reg.target.y = static_cast<double>(ly);
        reg.target.uri = uri;
      } else {
        int page_num = 0;
        if (uri[0] == '#' &&
            std::sscanf(uri.c_str(), "#page=%d", &page_num) == 1 && page_num >= 1) {
          reg.target.kind = TextLinkTargetKind::InternalPage;
          reg.target.page_1based = page_num;
        } else {
          reg.target.kind = TextLinkTargetKind::Uri;
          reg.target.uri = uri;
        }
      }
    }
    layer.regions.push_back(std::move(reg));
  }

  return layer;
#endif
}

namespace {

#if defined(THUMTOO_HAVE_MUPDF)

void flatten_outline(fz_outline* node, int level,
                     std::vector<std::tuple<int, std::string, std::string>>& out) {
  // No fz_try here — only walk the tree.
  for (; node; node = node->next) {
    std::string title = node->title ? node->title : "";
    std::string uri = node->uri ? node->uri : "";
    out.emplace_back(level, std::move(title), std::move(uri));
    if (node->down) flatten_outline(node->down, level + 1, out);
  }
}

void append_outline(fz_context* ctx, fz_document* doc, fz_outline* root, int /*level*/,
                    DocumentOutline& out) {
  std::vector<std::tuple<int, std::string, std::string>> snaps;
  flatten_outline(root, 1, snaps);
  for (std::size_t i = 0; i < snaps.size(); ++i) {
    const int level = std::get<0>(snaps[i]);
    const std::string title = std::get<1>(snaps[i]);
    const std::string uri = std::get<2>(snaps[i]);
    OutlineItem item;
    item.level = level;
    item.title = title;
    if (!uri.empty()) {
      int dest_page = -1;
      float lx = 0, ly = 0;
      if (resolve_internal_link_page(ctx, doc, uri.c_str(), &dest_page, &lx, &ly)) {
        item.page_1based = dest_page + 1;
        item.uri = uri;
      } else {
        item.uri = uri;
      }
      (void)lx;
      (void)ly;
    }
    out.items.push_back(std::move(item));
  }
}
#endif

}  // namespace

std::optional<DocumentOutline> mupdf_document_outline(
    const std::filesystem::path& path) {
#if !defined(THUMTOO_HAVE_MUPDF)
  (void)path;
  return std::nullopt;
#else
  DocAccess acc(path);
  if (!acc) return std::nullopt;
  fz_context* ctx = acc.ctx();
  fz_document* doc = acc.doc();

  fz_outline* root = nullptr;
  fz_var(root);
  fz_try(ctx) { root = fz_load_outline(ctx, doc); }
  fz_catch(ctx) { root = nullptr; }
  if (!root) {
    // Empty outline is valid (document has none).
    return DocumentOutline{};
  }
  DocumentOutline out;
  append_outline(ctx, doc, root, 1, out);
  fz_drop_outline(ctx, root);
  return out;
#endif
}


void set_smooth_image_scaling(bool on)
{
  g_smooth_image_scaling.store(on, std::memory_order_relaxed);
}

bool smooth_image_scaling()
{
  return g_smooth_image_scaling.load(std::memory_order_relaxed);
}

}  // namespace thumtoo
