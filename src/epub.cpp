// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "thumtoo/epub.hpp"
#include "thumtoo/format.hpp"
#include "thumtoo/uri.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <tuple>

#if defined(THUMTOO_HAVE_MUPDF)
#include <mupdf/fitz.h>
#endif

namespace thumtoo {
namespace {

#if defined(THUMTOO_HAVE_MUPDF)

struct TlsEpub {
  fz_context* ctx = nullptr;
  std::string path_key;
  std::filesystem::file_time_type mtime{};
  int width_px = 0;
  int height_px = 0;
  int fs_pt = 0;
  int mt_px = 0;
  int mr_px = 0;
  int mb_px = 0;
  int ml_px = 0;
  int lh_percent = 0;
  int cols = 1;
  int cgap_px = 0;
  int align = 0;
  int font = 0;
  int theme = 0;
  bool use_document_css = true;
  fz_document* doc = nullptr;
  bool laid_out = false;
  int page_index = -1;
  fz_page* page = nullptr;
  fz_display_list* list = nullptr;
};

thread_local TlsEpub g_tls;

void tls_drop_page() {
  if (!g_tls.ctx) return;
  if (g_tls.list) {
    fz_drop_display_list(g_tls.ctx, g_tls.list);
    g_tls.list = nullptr;
  }
  if (g_tls.page) {
    fz_drop_page(g_tls.ctx, g_tls.page);
    g_tls.page = nullptr;
  }
  g_tls.page_index = -1;
}

void tls_drop_doc() {
  tls_drop_page();
  if (g_tls.ctx && g_tls.doc) {
    fz_drop_document(g_tls.ctx, g_tls.doc);
    g_tls.doc = nullptr;
  }
  g_tls.path_key.clear();
  g_tls.laid_out = false;
  g_tls.width_px = 0;
  g_tls.height_px = 0;
  g_tls.fs_pt = 0;
  g_tls.mt_px = 0;
  g_tls.mr_px = 0;
  g_tls.mb_px = 0;
  g_tls.ml_px = 0;
  g_tls.lh_percent = 0;
  g_tls.cols = 1;
  g_tls.cgap_px = 0;
  g_tls.align = 0;
  g_tls.font = 0;
  g_tls.theme = 0;
  g_tls.use_document_css = true;
}

fz_context* tls_ctx() {
  if (!g_tls.ctx) {
    g_tls.ctx = fz_new_context(nullptr, nullptr, FZ_STORE_DEFAULT);
    if (g_tls.ctx) {
      fz_try(g_tls.ctx) { fz_register_document_handlers(g_tls.ctx); }
      fz_catch(g_tls.ctx) {
        fz_drop_context(g_tls.ctx);
        g_tls.ctx = nullptr;
      }
    }
  }
  return g_tls.ctx;
}

fz_document* tls_document(const std::filesystem::path& path,
                          const EpubLayout& layout) {
  fz_context* ctx = tls_ctx();
  if (!ctx) return nullptr;

  EpubLayout L = layout;
  if (L.width_px < 1) L.width_px = kEpubDefaultPageWidthPx;
  if (L.height_px < 1) L.height_px = kEpubDefaultPageHeightPx;
  if (L.fs_pt < 1) L.fs_pt = kEpubDefaultFontSizePt;
  if (L.mt_px < 0) L.mt_px = 0;
  if (L.mr_px < 0) L.mr_px = 0;
  if (L.mb_px < 0) L.mb_px = 0;
  if (L.ml_px < 0) L.ml_px = 0;
  if (L.lh_percent < 0) L.lh_percent = 0;
  if (L.cols < 1) L.cols = 1;
  if (L.cols > 6) L.cols = 6;
  if (L.cgap_px < 0) L.cgap_px = 0;

  std::error_code ec;
  const auto mtime = std::filesystem::last_write_time(path, ec);
  const std::string key = path.lexically_normal().string();
  const int font_i = static_cast<int>(L.font);
  const int theme_i = static_cast<int>(L.theme);
  const int align_i = static_cast<int>(L.align);
  if (g_tls.doc && g_tls.path_key == key && !ec && g_tls.mtime == mtime &&
      g_tls.width_px == L.width_px && g_tls.height_px == L.height_px &&
      g_tls.fs_pt == L.fs_pt && g_tls.mt_px == L.mt_px && g_tls.mr_px == L.mr_px &&
      g_tls.mb_px == L.mb_px && g_tls.ml_px == L.ml_px &&
      g_tls.lh_percent == L.lh_percent && g_tls.cols == L.cols &&
      g_tls.cgap_px == L.cgap_px && g_tls.align == align_i && g_tls.font == font_i &&
      g_tls.theme == theme_i && g_tls.use_document_css == L.use_document_css &&
      g_tls.laid_out) {
    return g_tls.doc;
  }

  tls_drop_doc();
  fz_document* doc = nullptr;
  fz_var(doc);
  fz_try(ctx) { doc = fz_open_document(ctx, path.string().c_str()); }
  fz_catch(ctx) { doc = nullptr; }
  if (!doc) return nullptr;

  // URI w/h/margins are pixels at kEpubLayoutDpi; MuPDF wants points.
  const float dpi = static_cast<float>(kEpubLayoutDpi);
  const float width_pt = static_cast<float>(L.width_px) * 72.f / dpi;
  const float height_pt = static_cast<float>(L.height_px) * 72.f / dpi;
  const float mt_pt = static_cast<float>(L.mt_px) * 72.f / dpi;
  const float mr_pt = static_cast<float>(L.mr_px) * 72.f / dpi;
  const float mb_pt = static_cast<float>(L.mb_px) * 72.f / dpi;
  const float ml_pt = static_cast<float>(L.ml_px) * 72.f / dpi;

  // Reader policy CSS (last in cascade). Font size always forced; optional
  // margins, line-height, family, theme colours.
  {
    std::string css;
    css.reserve(512);
    css += "html { font-size: ";
    css += std::to_string(L.fs_pt);
    css += "pt !important; }";
    css += "body { font-size: ";
    css += std::to_string(L.fs_pt);
    css += "pt !important; ";
    if (L.mt_px > 0 || L.mr_px > 0 || L.mb_px > 0 || L.ml_px > 0) {
      css += "margin: ";
      css += std::to_string(mt_pt);
      css += "pt ";
      css += std::to_string(mr_pt);
      css += "pt ";
      css += std::to_string(mb_pt);
      css += "pt ";
      css += std::to_string(ml_pt);
      css += "pt !important; ";
    } else {
      css += "margin: 0 !important; ";
    }
    if (L.lh_percent > 0) {
      css += "line-height: ";
      css += std::to_string(L.lh_percent / 100.0);
      css += " !important; ";
    }
    if (L.cols > 1) {
      css += "column-count: ";
      css += std::to_string(L.cols);
      css += " !important; column-fill: auto !important; ";
      if (L.cgap_px > 0) {
        const float gap_pt = static_cast<float>(L.cgap_px) * 72.f / dpi;
        css += "column-gap: ";
        css += std::to_string(gap_pt);
        css += "pt !important; ";
      }
    }
    if (L.align == EpubAlign::Left) {
      css += "text-align: left !important; ";
    } else if (L.align == EpubAlign::Right) {
      css += "text-align: right !important; ";
    } else if (L.align == EpubAlign::Center) {
      css += "text-align: center !important; ";
    } else if (L.align == EpubAlign::Justify) {
      css += "text-align: justify !important; ";
    }
    if (L.font == EpubFontFamily::Serif) {
      css += "font-family: serif !important; ";
    } else if (L.font == EpubFontFamily::Sans) {
      css += "font-family: sans-serif !important; ";
    } else if (L.font == EpubFontFamily::Mono) {
      css += "font-family: monospace !important; ";
    }
    if (L.theme == EpubTheme::Sepia) {
      css += "color: #5b4636 !important; background-color: #f4ecd8 !important; ";
    } else if (L.theme == EpubTheme::Night) {
      css += "color: #ddd !important; background-color: #1a1a1a !important; ";
    } else {
      css += "color: #111 !important; background-color: #fff !important; ";
    }
    css += "}";
    css += "p, li, td, th, div, span { font-size: inherit !important; ";
    if (L.lh_percent > 0) {
      css += "line-height: inherit !important; ";
    }
    if (L.font != EpubFontFamily::Publisher) {
      css += "font-family: inherit !important; ";
    }
    if (L.theme == EpubTheme::Sepia) {
      css += "color: inherit !important; background-color: transparent !important; ";
    } else if (L.theme == EpubTheme::Night) {
      css += "color: inherit !important; background-color: transparent !important; ";
    }
    css += "}";
    if (L.theme == EpubTheme::Night) {
      css += "a { color: #6af !important; }";
    } else if (L.theme == EpubTheme::Sepia) {
      css += "a { color: #396 !important; }";
    }
    // Per-document styles (MuPDF ≥ 1.28). Context globals fz_set_user_css /
    // fz_set_use_document_css are deprecated and spam warnings on every open.
    fz_style_document(ctx, doc, L.use_document_css ? 1 : 0, css.c_str());
  }

  int ok = 0;
  fz_var(ok);
  fz_try(ctx) {
    fz_layout_document(ctx, doc, width_pt, height_pt,
                       static_cast<float>(L.fs_pt));
    ok = 1;
  }
  fz_catch(ctx) { ok = 0; }
  if (!ok) {
    fz_drop_document(ctx, doc);
    return nullptr;
  }

  g_tls.path_key = key;
  g_tls.mtime = ec ? std::filesystem::file_time_type{} : mtime;
  g_tls.width_px = L.width_px;
  g_tls.height_px = L.height_px;
  g_tls.fs_pt = L.fs_pt;
  g_tls.mt_px = L.mt_px;
  g_tls.mr_px = L.mr_px;
  g_tls.mb_px = L.mb_px;
  g_tls.ml_px = L.ml_px;
  g_tls.lh_percent = L.lh_percent;
  g_tls.cols = L.cols;
  g_tls.cgap_px = L.cgap_px;
  g_tls.align = align_i;
  g_tls.font = font_i;
  g_tls.theme = theme_i;
  g_tls.use_document_css = L.use_document_css;
  g_tls.doc = doc;
  g_tls.laid_out = true;
  return doc;
}

fz_page* tls_page(const std::filesystem::path& path, const EpubLayout& layout,
                  int page_1based) {
  if (page_1based < 1) return nullptr;
  fz_context* ctx = tls_ctx();
  fz_document* doc = tls_document(path, layout);
  if (!ctx || !doc) return nullptr;

  const int idx = page_1based - 1;
  if (g_tls.page && g_tls.page_index == idx) return g_tls.page;

  tls_drop_page();
  int n = 0;
  fz_var(n);
  fz_try(ctx) { n = fz_count_pages(ctx, doc); }
  fz_catch(ctx) { n = 0; }
  if (idx < 0 || idx >= n) return nullptr;

  fz_page* page = nullptr;
  fz_var(page);
  fz_try(ctx) { page = fz_load_page(ctx, doc, idx); }
  fz_catch(ctx) { page = nullptr; }
  if (!page) return nullptr;

  g_tls.page = page;
  g_tls.page_index = idx;
  return page;
}

fz_display_list* tls_display_list(const std::filesystem::path& path,
                                  const EpubLayout& layout, int page_1based) {
  fz_context* ctx = tls_ctx();
  fz_page* page = tls_page(path, layout, page_1based);
  if (!ctx || !page) return nullptr;
  if (g_tls.list) return g_tls.list;

  fz_display_list* list = nullptr;
  fz_var(list);
  fz_try(ctx) { list = fz_new_display_list_from_page(ctx, page); }
  fz_catch(ctx) { list = nullptr; }
  g_tls.list = list;
  return list;
}

std::optional<PdfRaster> pixmap_to_rgb(fz_context* ctx, fz_pixmap* pix) {
  if (!ctx || !pix) return std::nullopt;
  const int w = fz_pixmap_width(ctx, pix);
  const int h = fz_pixmap_height(ctx, pix);
  const int n = fz_pixmap_components(ctx, pix);
  if (w <= 0 || h <= 0 || n < 3) return std::nullopt;

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
    } else if (n == 1 || n == 2) {
      for (int x = 0; x < w; ++x) {
        const unsigned char g = row[x * n];
        dst[x * 3 + 0] = g;
        dst[x * 3 + 1] = g;
        dst[x * 3 + 2] = g;
      }
    } else {
      return std::nullopt;
    }
  }
  return out;
}

#endif  // THUMTOO_HAVE_MUPDF

}  // namespace

bool epub_available() {
#if defined(THUMTOO_HAVE_MUPDF)
  return true;
#else
  return false;
#endif
}

bool is_likely_epub_path(const std::filesystem::path& path) {
  return is_epub_path(path);
}

std::optional<ParsedEpubUri> parse_epub_uri(std::string_view uri) {
  auto loc = parse_location(uri);
  if (!loc || loc->scheme != UriScheme::File) return std::nullopt;

  EpubLayout layout = default_epub_layout();
  int page = 0;
  bool saw_epub = false;
  for (const auto& pipe : loc->pipes) {
    if (pipe.kind == LocationPipeKind::EpubLayout) {
      layout = parse_epub_layout_params(pipe.value);
      saw_epub = true;
    } else if (pipe.kind == LocationPipeKind::PdfPage ||
               pipe.kind == LocationPipeKind::PdfPagePoppler ||
               pipe.kind == LocationPipeKind::PdfPageMupdf) {
      try {
        page = std::stoi(pipe.value);
      } catch (...) {
        return std::nullopt;
      }
    } else if (pipe.kind == LocationPipeKind::ArchiveRoot ||
               pipe.kind == LocationPipeKind::ArchiveMember) {
      return std::nullopt;
    }
  }
  if (!saw_epub || page < 1) return std::nullopt;

  std::filesystem::path path(loc->base);
  if (!is_epub_path(path)) return std::nullopt;

  ParsedEpubUri out;
  out.epub_path = std::move(path);
  out.layout = layout;
  out.page = page;
  return out;
}

std::string epub_page_uri(const std::filesystem::path& path, int page_1based,
                          const EpubLayout& layout) {
  auto base = with_epub_layout(file_uri_from_path(path.lexically_normal()), layout);
  return with_pdf_page(base, page_1based);
}

std::optional<int> epub_page_count(const std::filesystem::path& path,
                                   const EpubLayout& layout) {
#if !defined(THUMTOO_HAVE_MUPDF)
  (void)path;
  (void)layout;
  return std::nullopt;
#else
  fz_context* ctx = tls_ctx();
  fz_document* doc = tls_document(path, layout);
  if (!ctx || !doc) return std::nullopt;
  int n = 0;
  fz_var(n);
  fz_try(ctx) { n = fz_count_pages(ctx, doc); }
  fz_catch(ctx) { n = 0; }
  if (n <= 0) return std::nullopt;
  return n;
#endif
}

std::optional<Size> epub_page_layout_size(const std::filesystem::path& path,
                                          int page_1based,
                                          const EpubLayout& layout) {
#if !defined(THUMTOO_HAVE_MUPDF)
  (void)path;
  (void)page_1based;
  (void)layout;
  return std::nullopt;
#else
  fz_context* ctx = tls_ctx();
  fz_page* page = tls_page(path, layout, page_1based);
  if (!ctx || !page) return std::nullopt;
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
  const double scale = static_cast<double>(kEpubLayoutDpi) / 72.0;
  const int w =
      std::max(1, static_cast<int>(std::lround((box.x1 - box.x0) * scale)));
  const int h =
      std::max(1, static_cast<int>(std::lround((box.y1 - box.y0) * scale)));
  return Size{w, h};
#endif
}


std::optional<PdfRaster> epub_rasterize_page(const std::filesystem::path& path,
                                             int page_1based,
                                             const EpubLayout& layout,
                                             int max_edge) {
#if !defined(THUMTOO_HAVE_MUPDF)
  (void)path;
  (void)page_1based;
  (void)layout;
  (void)max_edge;
  return std::nullopt;
#else
  if (max_edge < 1 || page_1based < 1) return std::nullopt;
  fz_context* ctx = tls_ctx();
  fz_page* page = tls_page(path, layout, page_1based);
  if (!ctx || !page) return std::nullopt;

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
  const double pw = std::max(1.0, static_cast<double>(box.x1 - box.x0));
  const double ph = std::max(1.0, static_cast<double>(box.y1 - box.y0));
  const double long_pt = std::max(pw, ph);
  const double scale = static_cast<double>(max_edge) / long_pt;
  const int out_w = std::max(1, static_cast<int>(std::lround(pw * scale)));
  const int out_h = std::max(1, static_cast<int>(std::lround(ph * scale)));
  // dpi such that page maps to out_w x out_h
  const double dpi = 72.0 * scale;
  return epub_rasterize_page_region(path, page_1based, layout, dpi, 0, 0, out_w,
                                    out_h);
#endif
}

std::optional<PdfRaster> epub_rasterize_page_region(
    const std::filesystem::path& path, int page_1based, const EpubLayout& layout,
    double dpi, int px, int py, int pw, int ph) {
#if !defined(THUMTOO_HAVE_MUPDF)
  (void)path;
  (void)page_1based;
  (void)layout;
  (void)dpi;
  (void)px;
  (void)py;
  (void)pw;
  (void)ph;
  return std::nullopt;
#else
  if (page_1based < 1 || pw <= 0 || ph <= 0 || dpi <= 0.0) return std::nullopt;
  fz_context* ctx = tls_ctx();
  fz_display_list* list = tls_display_list(path, layout, page_1based);
  if (!ctx || !list) return std::nullopt;

  const float s = static_cast<float>(dpi / 72.0);
  fz_matrix ctm = fz_scale(s, s);
  fz_irect bbox;
  bbox.x0 = px;
  bbox.y0 = py;
  bbox.x1 = px + pw;
  bbox.y1 = py + ph;
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
  fz_try(ctx) {
    pix = fz_new_pixmap_with_bbox(ctx, fz_device_rgb(ctx), bbox, nullptr, 0);
    fz_clear_pixmap_with_value(ctx, pix, 0xff);
    dev = fz_new_draw_device(ctx, fz_identity, pix);
    fz_run_display_list(ctx, list, dev, ctm, clip, nullptr);
    fz_close_device(ctx, dev);
  }
  fz_always(ctx) {
    if (dev) {
      fz_drop_device(ctx, dev);
      dev = nullptr;
    }
  }
  fz_catch(ctx) {
    if (pix) {
      fz_drop_pixmap(ctx, pix);
      pix = nullptr;
    }
    failed = 1;
  }
  if (failed || !pix) return std::nullopt;
  auto out = pixmap_to_rgb(ctx, pix);
  fz_drop_pixmap(ctx, pix);
  return out;
#endif
}

std::optional<PdfRaster> epub_render_tile_cell(const std::filesystem::path& path,
                                               int page_1based,
                                               const EpubLayout& layout,
                                               int scale, int x, int y) {
  if (x < 0 || y < 0) return std::nullopt;
  auto layout_px = epub_page_layout_size(path, page_1based, layout);
  if (!layout_px || layout_px->width <= 0 || layout_px->height <= 0) {
    return std::nullopt;
  }
  const Size full = pdf_page_size_at_scale(*layout_px, scale);
  int left = 0, top = 0, tw = 0, th = 0;
  tile_cell_pixel_rect(full.width, full.height, x, y, &left, &top, &tw, &th);
  if (tw <= 0 || th <= 0) return std::nullopt;

  const double dpi =
      static_cast<double>(kEpubLayoutDpi) * std::ldexp(1.0, -scale);
  return epub_rasterize_page_region(path, page_1based, layout, dpi, left, top,
                                    tw, th);
}


namespace {

#if defined(THUMTOO_HAVE_MUPDF)
void append_utf8(std::string& out, int c) {
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


void flatten_epub_outline(fz_outline* node, int level,
                          std::vector<std::tuple<int, std::string, std::string>>& out) {
  for (; node; node = node->next) {
    std::string title = node->title ? node->title : "";
    std::string uri = node->uri ? node->uri : "";
    out.emplace_back(level, std::move(title), std::move(uri));
    if (node->down) flatten_epub_outline(node->down, level + 1, out);
  }
}

/** True for schemes that leave the document (open externally). */
[[nodiscard]] bool is_external_link_uri(const char* uri) {
  if (!uri || !uri[0]) return true;
  // Case-insensitive scheme check for common external protocols.
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

/**
 * Resolve an internal document link to a 0-based page via MuPDF.
 * Handles PDF #dest names and EPUB spine paths (e.g. Text/ch1.xhtml#frag).
 * External http(s)/mailto/… URIs return false.
 */
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

void append_epub_outline(fz_context* ctx, fz_document* doc, fz_outline* root,
                        int /*level*/, DocumentOutline& out) {
  std::vector<std::tuple<int, std::string, std::string>> snaps;
  flatten_epub_outline(root, 1, snaps);
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
        // Keep uri for diagnostics/tooltips; page wins for navigation.
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

std::optional<PageTextLayer> epub_page_text_layer(const std::filesystem::path& path,
                                                  int page_1based,
                                                  const EpubLayout& layout) {
#if !defined(THUMTOO_HAVE_MUPDF)
  (void)path;
  (void)page_1based;
  (void)layout;
  return std::nullopt;
#else
  if (page_1based < 1) return std::nullopt;
  fz_context* ctx = tls_ctx();
  fz_page* page = tls_page(path, layout, page_1based);
  if (!ctx || !page) return std::nullopt;

  PageTextLayer layer;
  layer.page_1based = page_1based;
  layer.layout_key = format_epub_layout_params(layout);

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
  // MuPDF page space for EPUB: top-left, Y down (same as PDF via MuPDF).
  layer.page_y_up = false;

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
          append_utf8(line_text, ch->c);
        }
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
  fz_document* doc_for_links = tls_document(path, layout);
  for (std::size_t i = 0; i < snaps.size(); ++i) {
    const TextRect bbox = snaps[i].bbox;
    const std::string uri = snaps[i].uri;
    TextRegion reg;
    reg.role = TextRegionRole::Link;
    reg.bbox = bbox;
    if (!uri.empty()) {
      int dest_page = -1;
      float lx = 0, ly = 0;
      if (resolve_internal_link_page(ctx, doc_for_links, uri.c_str(), &dest_page, &lx,
                                      &ly)) {
        reg.target.kind = TextLinkTargetKind::InternalPage;
        reg.target.page_1based = dest_page + 1;
        reg.target.x = static_cast<double>(lx);
        reg.target.y = static_cast<double>(ly);
        // Preserve original uri for debugging; navigation uses page.
        reg.target.uri = uri;
      } else {
        reg.target.kind = TextLinkTargetKind::Uri;
        reg.target.uri = uri;
      }
    }
    layer.regions.push_back(std::move(reg));
  }

  return layer;
#endif
}

std::optional<DocumentOutline> epub_document_outline(const std::filesystem::path& path,
                                                     const EpubLayout& layout) {
#if !defined(THUMTOO_HAVE_MUPDF)
  (void)path;
  (void)layout;
  return std::nullopt;
#else
  fz_context* ctx = tls_ctx();
  fz_document* doc = tls_document(path, layout);
  if (!ctx || !doc) return std::nullopt;

  fz_outline* root = nullptr;
  fz_var(root);
  fz_try(ctx) { root = fz_load_outline(ctx, doc); }
  fz_catch(ctx) { root = nullptr; }
  if (!root) return DocumentOutline{};

  DocumentOutline out;
  append_epub_outline(ctx, doc, root, 1, out);
  fz_drop_outline(ctx, root);
  return out;
#endif
}


}  // namespace thumtoo
