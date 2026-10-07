// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "thumtoo/ocr.hpp"

#include "thumtoo/djvu.hpp"
#include "thumtoo/epub.hpp"
#include "thumtoo/pdf.hpp"
#include "thumtoo/uri.hpp"
#include "thumtoo/archive.hpp"
#include "thumtoo/image.hpp"
#include "thumtoo/format.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <mutex>
#include <string>
#include <filesystem>

#if defined(THUMTOO_HAVE_TESSERACT)
#include <vips/vips.h>
#include <tesseract/baseapi.h>
#include <tesseract/resultiterator.h>
#endif

namespace thumtoo {
namespace {

constexpr int kDefaultOcrMaxEdge = 3000;

struct RgbPage {
  int width = 0;
  int height = 0;
  std::vector<std::uint8_t> rgb;  // RGB888
  TextRect page_bounds;
  /// Matches PageTextLayer::page_y_up — document pages Y-up, plain images Y-down.
  bool page_y_up = false;
  int page_1based = 0;
};


thread_local std::string g_ocr_last_error;

void set_ocr_error(std::string_view msg) {
  g_ocr_last_error.assign(msg);
}

#if defined(THUMTOO_HAVE_TESSERACT)
void clear_ocr_error() { g_ocr_last_error.clear(); }

/// Convert any VipsImage to contiguous RGB uchar for Tesseract.
[[nodiscard]] std::optional<RgbPage> vips_image_to_rgb_page(VipsImage* in) {
  if (!in) return std::nullopt;
  VipsImage* rgb = nullptr;
  if (vips_colourspace(in, &rgb, VIPS_INTERPRETATION_sRGB, nullptr) != 0 || !rgb) {
    set_ocr_error("vips_colourspace to sRGB failed");
    return std::nullopt;
  }
  if (vips_image_get_format(rgb) != VIPS_FORMAT_UCHAR) {
    VipsImage* casted = nullptr;
    if (vips_cast_uchar(rgb, &casted, nullptr) != 0 || !casted) {
      g_object_unref(rgb);
      set_ocr_error("vips_cast_uchar failed");
      return std::nullopt;
    }
    g_object_unref(rgb);
    rgb = casted;
  }
  int bands = vips_image_get_bands(rgb);
  if (bands > 3) {
    VipsImage* extr = nullptr;
    if (vips_extract_band(rgb, &extr, 0, "n", 3, nullptr) != 0 || !extr) {
      g_object_unref(rgb);
      set_ocr_error("vips_extract_band (drop alpha) failed");
      return std::nullopt;
    }
    g_object_unref(rgb);
    rgb = extr;
    bands = 3;
  }
  if (bands == 1) {
    // Greyscale → RGB by repeating the channel.
    VipsImage* joined = nullptr;
    VipsImage* ins[] = {rgb, rgb, rgb};
    if (vips_bandjoin(ins, &joined, 3, nullptr) != 0 || !joined) {
      g_object_unref(rgb);
      set_ocr_error("vips_bandjoin greyscale→RGB failed");
      return std::nullopt;
    }
    g_object_unref(rgb);
    rgb = joined;
    bands = 3;
  }
  if (bands == 2) {
    // LA → take L only then expand.
    VipsImage* L = nullptr;
    if (vips_extract_band(rgb, &L, 0, "n", 1, nullptr) != 0 || !L) {
      g_object_unref(rgb);
      set_ocr_error("vips_extract_band LA→L failed");
      return std::nullopt;
    }
    g_object_unref(rgb);
    VipsImage* joined = nullptr;
    VipsImage* ins[] = {L, L, L};
    if (vips_bandjoin(ins, &joined, 3, nullptr) != 0 || !joined) {
      g_object_unref(L);
      set_ocr_error("vips_bandjoin L→RGB failed");
      return std::nullopt;
    }
    g_object_unref(L);
    rgb = joined;
    bands = 3;
  }
  if (bands != 3 || vips_image_get_format(rgb) != VIPS_FORMAT_UCHAR) {
    g_object_unref(rgb);
    set_ocr_error("image is not 3-band uchar RGB after conversion");
    return std::nullopt;
  }
  const int w = vips_image_get_width(rgb);
  const int h = vips_image_get_height(rgb);
  if (w < 1 || h < 1) {
    g_object_unref(rgb);
    set_ocr_error("empty image dimensions");
    return std::nullopt;
  }
  size_t len = 0;
  void* data = vips_image_write_to_memory(rgb, &len);
  g_object_unref(rgb);
  if (!data || len == 0) {
    if (data) g_free(data);
    set_ocr_error("vips_image_write_to_memory failed");
    return std::nullopt;
  }
  const size_t need = static_cast<size_t>(w) * static_cast<size_t>(h) * 3;
  if (len < need) {
    g_free(data);
    set_ocr_error("RGB buffer shorter than width*height*3");
    return std::nullopt;
  }
  RgbPage out;
  out.width = w;
  out.height = h;
  out.rgb.resize(need);
  std::memcpy(out.rgb.data(), data, need);
  g_free(data);
  out.page_bounds = TextRect{0, 0, static_cast<double>(w), static_cast<double>(h)};
  out.page_y_up = false;  // source pixels, top-left
  out.page_1based = 1;
  return out;
}

[[nodiscard]] std::optional<RgbPage> vips_file_to_rgb_page(
    const std::filesystem::path& path, int max_edge) {
  image_library_init();
  int full_w = 0;
  int full_h = 0;
  if (auto probe = probe_image_file(path)) {
    full_w = probe->size.width;
    full_h = probe->size.height;
  }
  VipsImage* thumb = nullptr;
  if (vips_thumbnail(path.string().c_str(), &thumb, max_edge, "size",
                     VIPS_SIZE_DOWN, nullptr) != 0 ||
      !thumb) {
    set_ocr_error(std::string("vips_thumbnail failed for ") + path.string());
    return std::nullopt;
  }
  auto page = vips_image_to_rgb_page(thumb);
  g_object_unref(thumb);
  if (!page) return std::nullopt;
  // Page space = full upright image pixels (biltoo source size). Tess boxes are
  // in OCR-raster pixels; run_tesseract maps with uniform long-edge scale into
  // page_bounds.
  if (full_w > 0 && full_h > 0) {
    page->page_bounds =
        TextRect{0, 0, static_cast<double>(full_w), static_cast<double>(full_h)};
  }
  return page;
}

[[nodiscard]] std::optional<RgbPage> vips_buffer_to_rgb_page(const void* data,
                                                            size_t size,
                                                            int max_edge) {
  if (!data || size == 0) {
    set_ocr_error("empty image buffer");
    return std::nullopt;
  }
  image_library_init();
  VipsImage* thumb = nullptr;
  if (vips_thumbnail_buffer(const_cast<void*>(data), size, &thumb, max_edge,
                            "size", VIPS_SIZE_DOWN, nullptr) != 0 ||
      !thumb) {
    set_ocr_error("vips_thumbnail_buffer failed");
    return std::nullopt;
  }
  auto page = vips_image_to_rgb_page(thumb);
  g_object_unref(thumb);
  return page;
}

[[nodiscard]] std::optional<RgbPage> rasterize_uri_for_ocr(std::string_view uri,
                                                           int max_edge) {
  if (max_edge < 64) max_edge = kDefaultOcrMaxEdge;

  if (auto pdf = parse_pdf_uri(uri)) {
    auto layout = pdf_page_layout_size(pdf->pdf_path, pdf->page, pdf->backend);
    if (!layout || layout->width < 1 || layout->height < 1) return std::nullopt;
    // Page bounds in PDF user space (same as native text layer).
    auto bounds72 = pdf_page_size_72dpi(pdf->pdf_path, pdf->page, pdf->backend);
    TextRect bounds;
    if (bounds72) {
      bounds = TextRect{0, 0, static_cast<double>(bounds72->width),
                        static_cast<double>(bounds72->height)};
    } else {
      // Fallback: treat layout pixels as the page box (Y-down).
      bounds = TextRect{0, 0, static_cast<double>(layout->width),
                        static_cast<double>(layout->height)};
    }
    auto raster =
        pdf_rasterize_page(pdf->pdf_path, pdf->page, max_edge, pdf->backend);
    if (!raster || raster->rgb.empty() || raster->width < 1 ||
        raster->height < 1) {
      set_ocr_error(std::string("PDF rasterize failed: ") +
                    pdf->pdf_path.string() + " page=" +
                    std::to_string(pdf->page));
      return std::nullopt;
    }
    RgbPage out;
    out.width = raster->width;
    out.height = raster->height;
    out.rgb = std::move(raster->rgb);
    out.page_bounds = bounds;
    out.page_y_up = false;  // MuPDF page+stext space is top-left Y-down (native PDF layer)
    out.page_1based = pdf->page;
    return out;
  }

#if defined(THUMTOO_HAVE_DJVU)
  if (auto dj = parse_djvu_uri(uri)) {
    if (is_likely_djvu_path(dj->djvu_path)) {
      auto layout = djvu_page_layout_size(dj->djvu_path, dj->page);
      if (!layout || layout->width < 1 || layout->height < 1) return std::nullopt;
      auto raster = djvu_rasterize_page(dj->djvu_path, dj->page, max_edge);
      if (!raster || raster->rgb.empty()) return std::nullopt;
      RgbPage out;
      out.width = raster->width;
      out.height = raster->height;
      out.rgb = std::move(raster->rgb);
      out.page_bounds =
          TextRect{0, 0, static_cast<double>(layout->width),
                   static_cast<double>(layout->height)};
      out.page_y_up = true;  // same as native DjVu text (bottom-left)
      out.page_1based = dj->page;
      return out;
    }
  }
#endif


#if defined(THUMTOO_HAVE_MUPDF)
  if (auto ep = parse_epub_uri(uri)) {
    if (ep->page < 1) {
      set_ocr_error(std::string("EPUB URI missing page pipe: ") + std::string(uri));
      return std::nullopt;
    }
    auto layout = epub_page_layout_size(ep->epub_path, ep->page, ep->layout);
    if (!layout || layout->width < 1 || layout->height < 1) {
      set_ocr_error(std::string("EPUB page layout failed: ") + ep->epub_path.string() +
                    " page=" + std::to_string(ep->page));
      return std::nullopt;
    }
    auto raster =
        epub_rasterize_page(ep->epub_path, ep->page, ep->layout, max_edge);
    if (!raster || raster->rgb.empty() || raster->width < 1 || raster->height < 1) {
      set_ocr_error(std::string("EPUB page rasterize failed: ") +
                    ep->epub_path.string() + " page=" + std::to_string(ep->page));
      return std::nullopt;
    }
    RgbPage out;
    out.width = raster->width;
    out.height = raster->height;
    out.rgb = std::move(raster->rgb);
    // Layout-pixel page box; Y-down to match MuPDF native EPUB text layer.
    out.page_bounds =
        TextRect{0, 0, static_cast<double>(layout->width),
                 static_cast<double>(layout->height)};
    out.page_y_up = false;
    out.page_1based = ep->page;
    return out;
  }
#endif

  // Archive member image: file://…//archive:member
  if (auto arch = parse_archive_uri(uri)) {
    if (!arch->member_path.empty()) {
      auto bytes = extract_archive_member(arch->archive_path, arch->member_path);
      if (!bytes || bytes->empty()) {
        set_ocr_error("archive member extract failed");
        return std::nullopt;
      }
      return vips_buffer_to_rgb_page(bytes->data(), bytes->size(), max_edge);
    }
  }

  // Plain image files (non multipage containers).
  if (auto path = path_from_file_uri(uri)) {
    if (is_mupdf_page_document_path(*path) || is_djvu_path(*path) || is_epub_path(*path)) {
      set_ocr_error(std::string("multipage document without usable page pipe: ") +
                    std::string(uri));
      return std::nullopt;
    }
    if (is_likely_archive_path(*path)) {
      set_ocr_error("path is an archive without //archive:member");
      return std::nullopt;
    }
    return vips_file_to_rgb_page(*path, max_edge);
  }

  set_ocr_error(std::string("unsupported URI for OCR rasterize: ") +
                std::string(uri));
  return std::nullopt;
}

[[nodiscard]] std::string tesseract_version_string() {
  const char* v = tesseract::TessBaseAPI::Version();
  return v ? std::string(v) : std::string{};
}

std::mutex g_tess_mu;


/// Clamp Tesseract source DPI into a range where the layout analysis is stable.
[[nodiscard]] int clamp_ocr_dpi(int dpi) {
  if (dpi < 70) return 70;
  if (dpi > 600) return 600;
  return dpi;
}

/**
 * Estimate source DPI for Tesseract.
 *
 * - opts.dpi > 0: host override (clamped).
 * - Document page space (page_y_up, bounds not equal to pixel box): treat
 *   page_bounds as PDF-style points (72/inch) → dpi = 72 * pix / bounds.
 * - Pixel page box (bounds ≈ image size, typical for plain images / RGB host
 *   buffers without a media box): default 300 so mixed body/caption sizes
 *   segment reasonably. (72 would claim the buffer is ~1" wide.)
 */
[[nodiscard]] int estimate_ocr_dpi(const RgbPage& page, const OcrOptions& opts) {
  if (opts.dpi > 0) {
    return clamp_ocr_dpi(opts.dpi);
  }
  const double bw = page.page_bounds.width();
  const double bh = page.page_bounds.height();
  if (bw <= 1.0 || bh <= 1.0 || page.width < 1 || page.height < 1) {
    return 300;
  }
  const double pix_w = static_cast<double>(page.width);
  const double pix_h = static_cast<double>(page.height);
  // Pixel page box: bounds match the raster (within 1 unit).
  const bool pixel_box =
      std::abs(bw - pix_w) < 1.5 && std::abs(bh - pix_h) < 1.5 &&
      std::abs(page.page_bounds.x0) < 1.5 && std::abs(page.page_bounds.y0) < 1.5;
  if (pixel_box && !page.page_y_up) {
    return 300;
  }
  // Document page box in points (or DjVu pixels with Y-up): scale from width.
  const int from_w =
      static_cast<int>(std::lround(72.0 * pix_w / bw));
  // DjVu Y-up page_bounds are often already in pixels (same magnitude as
  // raster). If the implied DPI is absurd (>600 or <70), fall back to 300.
  if (from_w < 70 || from_w > 600) {
    return 300;
  }
  return from_w;
}

[[nodiscard]] std::optional<PageTextLayer> run_tesseract(
    const RgbPage& page, const OcrOptions& opts) {
  std::lock_guard<std::mutex> lock(g_tess_mu);
  tesseract::TessBaseAPI api;
  const std::string lang = opts.lang.empty() ? "eng" : opts.lang;
  if (api.Init(nullptr, lang.c_str()) != 0) {
    set_ocr_error(std::string("Tesseract Init failed for lang=") + lang +
                  " (missing tessdata / TESSDATA_PREFIX?)");
    return std::nullopt;
  }
  api.SetPageSegMode(tesseract::PSM_AUTO);
  api.SetImage(page.rgb.data(), page.width, page.height, 3, page.width * 3);
  // Tesseract defaults to ~70 DPI when unset; that mis-scales mixed body /
  // caption / header sizes (especially host appearance / crop bitmaps).
  const int dpi = estimate_ocr_dpi(page, opts);
  api.SetSourceResolution(dpi);
  if (api.Recognize(nullptr) != 0) {
    api.End();
    set_ocr_error("Tesseract Recognize failed");
    return std::nullopt;
  }

  PageTextLayer layer;
  layer.page_1based = page.page_1based;
  layer.page_bounds = page.page_bounds;
  layer.page_y_up = page.page_y_up;
  layer.source = TextLayerSource::Ocr;

  OcrMeta meta;
  meta.engine = opts.engine.empty() ? "tesseract" : opts.engine;
  meta.engine_version = tesseract_version_string();
  meta.model = opts.model.empty() ? "default" : opts.model;
  meta.lang = lang;
  meta.dpi = dpi;
  meta.created_unix = std::chrono::duration_cast<std::chrono::seconds>(
                          std::chrono::system_clock::now().time_since_epoch())
                          .count();
  meta.params.emplace_back("psm", "auto");
  meta.params.emplace_back("dpi", std::to_string(dpi));
  layer.ocr = meta;

  const double pw = page.page_bounds.width();
  const double ph = page.page_bounds.height();
  const double ox = page.page_bounds.x0;
  const double oy = page.page_bounds.y0;
  // Uniform scale from the *fit* that produced the OCR raster (long-edge
  // match). Using independent sx/sy from integer width/height lets aspect
  // rounding accumulate along long lines ("drift").
  const double page_long = std::max(pw, ph);
  const double pix_long =
      static_cast<double>(std::max(page.width, page.height));
  const double unit_per_px =
      (pix_long > 0.0 && page_long > 0.0) ? (page_long / pix_long)
                                          : 1.0;
  const double sx = unit_per_px;
  const double sy = unit_per_px;
  // If the raster is letterboxed inside page_bounds (should not happen with
  // our fit), centre the pixel grid in page space.
  const double raster_w_units = static_cast<double>(page.width) * unit_per_px;
  const double raster_h_units = static_cast<double>(page.height) * unit_per_px;
  const double pad_x = 0.5 * (pw - raster_w_units);
  const double pad_y = 0.5 * (ph - raster_h_units);
  const double origin_x = ox + pad_x;
  const double origin_y = oy + pad_y;

  tesseract::ResultIterator* ri = api.GetIterator();
  if (ri != nullptr) {
    int block_id = 0;
    // Line-level regions (good for Find / select).
    do {
      const char* utf8 = ri->GetUTF8Text(tesseract::RIL_TEXTLINE);
      if (!utf8) continue;
      std::string text(utf8);
      delete[] utf8;
      while (!text.empty() &&
             (text.back() == '\n' || text.back() == '\r' || text.back() == ' ')) {
        text.pop_back();
      }
      if (text.empty()) continue;

      int left = 0, top = 0, right = 0, bottom = 0;
      if (!ri->BoundingBox(tesseract::RIL_TEXTLINE, &left, &top, &right,
                           &bottom)) {
        continue;
      }
      if (right <= left || bottom <= top) continue;

      TextRegion reg;
      reg.role = TextRegionRole::Text;
      reg.text = std::move(text);
      reg.bbox.x0 = origin_x + static_cast<double>(left) * sx;
      reg.bbox.x1 = origin_x + static_cast<double>(right) * sx;
      // Tesseract boxes are top-left Y-down in the OCR raster. Map into page
      // space: Y-up document pages flip about page_bounds; plain images stay
      // top-left Y-down.
      if (page.page_y_up) {
        const double y_top =
            oy + ph - pad_y - static_cast<double>(top) * sy;
        const double y_bot =
            oy + ph - pad_y - static_cast<double>(bottom) * sy;
        reg.bbox.y0 = std::min(y_top, y_bot);
        reg.bbox.y1 = std::max(y_top, y_bot);
      } else {
        reg.bbox.y0 = origin_y + static_cast<double>(top) * sy;
        reg.bbox.y1 = origin_y + static_cast<double>(bottom) * sy;
      }
      if (ri->IsAtBeginningOf(tesseract::RIL_BLOCK)) {
        ++block_id;
      }
      reg.block_id = block_id;
      layer.regions.push_back(std::move(reg));
    } while (ri->Next(tesseract::RIL_TEXTLINE));
  }

  api.End();
  return layer;
}

#endif  // THUMTOO_HAVE_TESSERACT

}  // namespace

bool ocr_available() {
#if defined(THUMTOO_HAVE_TESSERACT)
  return true;
#else
  return false;
#endif
}

std::string_view ocr_last_error() {
  return g_ocr_last_error;
}

#if defined(THUMTOO_HAVE_TESSERACT)
/// Crop OCR raster to opts crop (page space). @p full_bounds keeps the
/// original page box for the returned layer; @p page.page_bounds becomes the
/// crop so Tesseract pixel→page mapping stays in absolute page coordinates.
[[nodiscard]] bool apply_ocr_page_crop(RgbPage& page, const OcrOptions& opts,
                                       TextRect* full_bounds_out) {
  if (!opts.has_crop) {
    return true;
  }
  const double cx0 = std::min(opts.crop_x0, opts.crop_x1);
  const double cy0 = std::min(opts.crop_y0, opts.crop_y1);
  const double cx1 = std::max(opts.crop_x0, opts.crop_x1);
  const double cy1 = std::max(opts.crop_y0, opts.crop_y1);
  if (!(cx1 > cx0) || !(cy1 > cy0)) {
    set_ocr_error("OCR crop rect is empty");
    return false;
  }
  const double pb_x0 = page.page_bounds.x0;
  const double pb_y0 = page.page_bounds.y0;
  const double pb_x1 = page.page_bounds.x1;
  const double pb_y1 = page.page_bounds.y1;
  const double pb_w = pb_x1 - pb_x0;
  const double pb_h = pb_y1 - pb_y0;
  if (!(pb_w > 0.0) || !(pb_h > 0.0) || page.width < 1 || page.height < 1) {
    set_ocr_error("OCR page bounds invalid for crop");
    return false;
  }
  const double ix0 = std::max(cx0, pb_x0);
  const double iy0 = std::max(cy0, pb_y0);
  const double ix1 = std::min(cx1, pb_x1);
  const double iy1 = std::min(cy1, pb_y1);
  if (!(ix1 > ix0) || !(iy1 > iy0)) {
    set_ocr_error("OCR crop does not intersect page");
    return false;
  }

  // Match run_tesseract long-edge scale + letterbox pad.
  const double page_long = std::max(pb_w, pb_h);
  const double pix_long =
      static_cast<double>(std::max(page.width, page.height));
  const double unit_per_px =
      (pix_long > 0.0 && page_long > 0.0) ? (page_long / pix_long) : 1.0;
  const double raster_w_units = static_cast<double>(page.width) * unit_per_px;
  const double raster_h_units = static_cast<double>(page.height) * unit_per_px;
  const double pad_x = 0.5 * (pb_w - raster_w_units);
  const double pad_y = 0.5 * (pb_h - raster_h_units);
  const double origin_x = pb_x0 + pad_x;
  const double origin_y = pb_y0 + pad_y;

  int left = static_cast<int>(std::floor((ix0 - origin_x) / unit_per_px));
  int right = static_cast<int>(std::ceil((ix1 - origin_x) / unit_per_px));
  int top = 0;
  int bottom = 0;
  if (page.page_y_up) {
    // High page Y is the top of the raster.
    const double top_page_y = pb_y0 + pb_h - pad_y;  // image row 0
    top = static_cast<int>(std::floor((top_page_y - iy1) / unit_per_px));
    bottom = static_cast<int>(std::ceil((top_page_y - iy0) / unit_per_px));
  } else {
    top = static_cast<int>(std::floor((iy0 - origin_y) / unit_per_px));
    bottom = static_cast<int>(std::ceil((iy1 - origin_y) / unit_per_px));
  }
  left = std::max(0, std::min(left, page.width - 1));
  top = std::max(0, std::min(top, page.height - 1));
  right = std::max(left + 1, std::min(right, page.width));
  bottom = std::max(top + 1, std::min(bottom, page.height));
  const int cw = right - left;
  const int ch = bottom - top;
  if (cw < 8 || ch < 8) {
    set_ocr_error("OCR crop is too small after raster mapping");
    return false;
  }

  std::vector<std::uint8_t> cropped(static_cast<size_t>(cw * ch * 3));
  for (int y = 0; y < ch; ++y) {
    const std::uint8_t* src =
        page.rgb.data() + (static_cast<size_t>(top + y) * page.width + left) * 3;
    std::uint8_t* dst = cropped.data() + static_cast<size_t>(y) * cw * 3;
    std::memcpy(dst, src, static_cast<size_t>(cw) * 3);
  }
  page.rgb = std::move(cropped);
  page.width = cw;
  page.height = ch;
  if (full_bounds_out) {
    *full_bounds_out = page.page_bounds;
  }
  // Tesseract maps (0,0) pixel → crop origin in page space.
  page.page_bounds = TextRect{ix0, iy0, ix1, iy1};
  return true;
}
#endif  // THUMTOO_HAVE_TESSERACT

std::optional<PageTextLayer> ocr_page_text_layer(std::string_view uri,
                                                 const OcrOptions& opts) {
#if !defined(THUMTOO_HAVE_TESSERACT)
  (void)uri;
  (void)opts;
  set_ocr_error("Tesseract not compiled into thumtoo");
  return std::nullopt;
#else
  clear_ocr_error();
  if (uri.empty()) {
    set_ocr_error("empty URI");
    return std::nullopt;
  }
  const int max_edge =
      opts.max_edge > 0 ? opts.max_edge : kDefaultOcrMaxEdge;
  auto page = rasterize_uri_for_ocr(uri, max_edge);
  if (!page) {
    if (g_ocr_last_error.empty()) set_ocr_error("rasterize failed");
    return std::nullopt;
  }
  TextRect full_bounds = page->page_bounds;
  if (opts.has_crop) {
    if (!apply_ocr_page_crop(*page, opts, &full_bounds)) {
      return std::nullopt;
    }
  }
  auto layer = run_tesseract(*page, opts);
  if (!layer) {
    if (g_ocr_last_error.empty()) set_ocr_error("Tesseract failed");
    return std::nullopt;
  }
  // Keep full page bounds for consumers mapping overlays onto the page.
  layer->page_bounds = full_bounds;
  clear_ocr_error();
  // layout_key filled by Client when storing (native base + ocr suffix).
  return layer;
#endif
}

std::optional<PageTextLayer> ocr_rgb_page_text_layer(
    const std::uint8_t* rgb, int width, int height,
    const TextRect& page_bounds, const OcrOptions& opts) {
#if !defined(THUMTOO_HAVE_TESSERACT)
  (void)rgb;
  (void)width;
  (void)height;
  (void)page_bounds;
  (void)opts;
  set_ocr_error("Tesseract not compiled into thumtoo");
  return std::nullopt;
#else
  clear_ocr_error();
  if (!rgb || width < 8 || height < 8) {
    set_ocr_error("OCR RGB buffer too small or null");
    return std::nullopt;
  }
  if (page_bounds.width() <= 0.0 || page_bounds.height() <= 0.0) {
    set_ocr_error("OCR page_bounds invalid");
    return std::nullopt;
  }
  RgbPage page;
  page.width = width;
  page.height = height;
  page.page_bounds = page_bounds;
  // Caller buffer is top-left Y-down (host appearance path remaps to page space).
  page.page_y_up = false;
  page.page_1based = 0;
  page.rgb.assign(rgb, rgb + static_cast<size_t>(width) * height * 3);

  // Optional long-edge shrink for large host buffers.
  const int max_edge = opts.max_edge > 0 ? opts.max_edge : kDefaultOcrMaxEdge;
  const int long_edge = std::max(width, height);
  if (long_edge > max_edge) {
    const double scale = static_cast<double>(max_edge) / static_cast<double>(long_edge);
    const int nw = std::max(8, static_cast<int>(std::lround(width * scale)));
    const int nh = std::max(8, static_cast<int>(std::lround(height * scale)));
    // Nearest-neighbor shrink (host already graded; quality secondary to speed).
    std::vector<std::uint8_t> small(static_cast<size_t>(nw * nh * 3));
    for (int y = 0; y < nh; ++y) {
      const int sy = std::min(height - 1,
                              static_cast<int>(std::lround(y / scale)));
      for (int x = 0; x < nw; ++x) {
        const int sx = std::min(width - 1,
                                static_cast<int>(std::lround(x / scale)));
        const std::uint8_t* s =
            page.rgb.data() + (static_cast<size_t>(sy) * width + sx) * 3;
        std::uint8_t* d = small.data() + (static_cast<size_t>(y) * nw + x) * 3;
        d[0] = s[0];
        d[1] = s[1];
        d[2] = s[2];
      }
    }
    page.rgb = std::move(small);
    page.width = nw;
    page.height = nh;
    // page_bounds unchanged — run_tesseract scales uniformly into it.
  }

  auto layer = run_tesseract(page, opts);
  if (!layer) {
    if (g_ocr_last_error.empty()) set_ocr_error("Tesseract failed");
    return std::nullopt;
  }
  clear_ocr_error();
  return layer;
#endif
}

}  // namespace thumtoo
