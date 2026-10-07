// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

// EPUB on the shared MuPDF runtime (src/mupdf_runtime.hpp): one laid-out
// document per (file, layout), cached display lists, page profiles,
// decode-once images and render stats — the same machinery as PDF. The
// layout (page box, CSS policy) is the runtime's OpenSpec.

#include "thumtoo/epub.hpp"
#include "thumtoo/format.hpp"
#include "thumtoo/uri.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <tuple>
#include <vector>

#if defined(THUMTOO_HAVE_MUPDF)
#include "mupdf_runtime.hpp"
#endif

namespace thumtoo {
namespace {

#if defined(THUMTOO_HAVE_MUPDF)

/// The runtime's open recipe for @p layout: page box in points, base font
/// size, and the reader-policy CSS (last in the cascade).
mupdf::OpenSpec open_spec(const EpubLayout& layout) {
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
  std::string css;
  {
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
  }
  mupdf::OpenSpec spec;
  spec.key = format_epub_layout_params(L);
  spec.css = std::move(css);
  spec.use_document_css = L.use_document_css;
  spec.width_pt = width_pt;
  spec.height_pt = height_pt;
  spec.em = static_cast<float>(L.fs_pt);
  return spec;
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
  const auto spec = open_spec(layout);
  return mupdf::page_count(path, &spec);
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
  const auto spec = open_spec(layout);
  return mupdf::page_layout_size(path, &spec, page_1based);
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
  const auto spec = open_spec(layout);
  return mupdf::rasterize_page(path, &spec, page_1based, max_edge);
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
  const auto spec = open_spec(layout);
  return mupdf::rasterize_region(path, &spec, page_1based, dpi, px, py, pw, ph);
#endif
}

PdfCellRender epub_render_tile_cell(const std::filesystem::path& path, int page_1based,
                                    const EpubLayout& layout, int scale, int x, int y) {
#if !defined(THUMTOO_HAVE_MUPDF)
  (void)path;
  (void)page_1based;
  (void)layout;
  (void)scale;
  (void)x;
  (void)y;
  return {TileStatus::Failed, std::nullopt, "EPUB needs MuPDF"};
#else
  const auto spec = open_spec(layout);
  return mupdf::render_cell(path, &spec, page_1based, scale, x, y);
#endif
}

std::optional<PdfPageProfile> epub_page_profile(const std::filesystem::path& path,
                                                int page_1based, const EpubLayout& layout,
                                                std::string* error) {
#if !defined(THUMTOO_HAVE_MUPDF)
  (void)path;
  (void)page_1based;
  (void)layout;
  if (error) *error = "EPUB needs MuPDF";
  return std::nullopt;
#else
  const auto spec = open_spec(layout);
  return mupdf::page_profile(path, &spec, page_1based, error);
#endif
}

std::optional<PdfDocumentRenderStats> epub_document_render_stats(
    const std::filesystem::path& path) {
  // One registry for every MuPDF-backed format, keyed by file.
  return pdf_document_render_stats(path);
}

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
  const auto spec = open_spec(layout);
  std::optional<PageTextLayer> result;
  mupdf::with_document(path, &spec, page_1based,
                       [&](fz_context* ctx, fz_document* doc, fz_page* page) {
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
  if (!ok) return;
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
          utf8_append_codepoint(line_text, ch->c);
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
  fz_document* doc_for_links = doc;
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

  result = std::move(layer);
                       });
  return result;
#endif
}

std::optional<DocumentOutline> epub_document_outline(const std::filesystem::path& path,
                                                     const EpubLayout& layout) {
#if !defined(THUMTOO_HAVE_MUPDF)
  (void)path;
  (void)layout;
  return std::nullopt;
#else
  const auto spec = open_spec(layout);
  std::optional<DocumentOutline> result;
  mupdf::with_document(path, &spec, 0, [&](fz_context* ctx, fz_document* doc, fz_page*) {
    fz_outline* root = nullptr;
    fz_var(root);
    fz_try(ctx) { root = fz_load_outline(ctx, doc); }
    fz_catch(ctx) { root = nullptr; }
    DocumentOutline out;
    if (root) {
      append_epub_outline(ctx, doc, root, 1, out);
      fz_drop_outline(ctx, root);
    }
    result = std::move(out);
  });
  return result;
#endif
}

}  // namespace thumtoo
