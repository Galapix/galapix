// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "pdf_fixtures.hpp"

#include <cstdio>
#include <functional>
#include <string>
#include <vector>

#include <mupdf/fitz.h>
#include <mupdf/pdf.h>

namespace thumtoo::fixtures {

namespace {

// 2 x 2 inch pages: an N px wide full-page image is N/2 dpi.
constexpr float kPagePt = 144.0f;

struct Builder {
  fz_context* ctx = nullptr;
  pdf_document* doc = nullptr;
  std::vector<pdf_obj*> keep;  // objects dropped at the end

  pdf_obj* font() {
    if (!font_ref) {
      fz_font* f = fz_new_base14_font(ctx, "Helvetica");
      font_ref = pdf_add_simple_font(ctx, doc, f, PDF_SIMPLE_ENCODING_LATIN);
      fz_drop_font(ctx, f);
      keep.push_back(font_ref);
    }
    return font_ref;
  }

  /// Gray (n=1) or RGB (n=3) pattern image as an XObject reference.
  pdf_obj* image(int w, int h, int n) {
    fz_colorspace* cs = n == 1 ? fz_device_gray(ctx) : fz_device_rgb(ctx);
    fz_pixmap* pix = fz_new_pixmap(ctx, cs, w, h, nullptr, 0);
    unsigned char* p = fz_pixmap_samples(ctx, pix);
    const int stride = fz_pixmap_stride(ctx, pix);
    for (int y = 0; y < h; ++y) {
      for (int x = 0; x < w; ++x) {
        for (int c = 0; c < n; ++c) {
          p[y * stride + x * n + c] =
              static_cast<unsigned char>(((x * 7) ^ (y * 3)) + c * 40);
        }
      }
    }
    fz_image* img = fz_new_image_from_pixmap(ctx, pix, nullptr);
    fz_drop_pixmap(ctx, pix);
    pdf_obj* ref = pdf_add_image(ctx, doc, img);
    fz_drop_image(ctx, img);
    keep.push_back(ref);
    return ref;
  }

  /// 1-bit stencil (/ImageMask true) — how bitonal scans and JBIG2 text
  /// layers are drawn.
  pdf_obj* stencil(int w, int h) {
    const int row = (w + 7) / 8;
    fz_buffer* buf = fz_new_buffer(ctx, static_cast<size_t>(row) * h);
    for (int y = 0; y < h; ++y) {
      for (int b = 0; b < row; ++b) {
        fz_append_byte(ctx, buf, ((b + y / 8) & 1) ? 0xAA : 0x0F);
      }
    }
    pdf_obj* dict = pdf_new_dict(ctx, doc, 6);
    pdf_dict_put(ctx, dict, PDF_NAME(Type), PDF_NAME(XObject));
    pdf_dict_put(ctx, dict, PDF_NAME(Subtype), PDF_NAME(Image));
    pdf_dict_put_int(ctx, dict, PDF_NAME(Width), w);
    pdf_dict_put_int(ctx, dict, PDF_NAME(Height), h);
    pdf_dict_put_bool(ctx, dict, PDF_NAME(ImageMask), 1);
    pdf_dict_put_int(ctx, dict, PDF_NAME(BitsPerComponent), 1);
    pdf_obj* ref = pdf_add_stream(ctx, doc, buf, dict, 0);
    pdf_drop_obj(ctx, dict);
    fz_drop_buffer(ctx, buf);
    keep.push_back(ref);
    return ref;
  }

  /// Add a page with @p xobjects (/Im0, /Im1, …) and the Helvetica font as /F1.
  void page(const std::string& content, std::vector<pdf_obj*> xobjects,
            float w = kPagePt, float h = kPagePt, int rotate = 0) {
    pdf_obj* res = pdf_new_dict(ctx, doc, 2);
    pdf_obj* fonts = pdf_dict_put_dict(ctx, res, PDF_NAME(Font), 1);
    pdf_dict_puts(ctx, fonts, "F1", font());
    if (!xobjects.empty()) {
      pdf_obj* xo = pdf_dict_put_dict(ctx, res, PDF_NAME(XObject), 2);
      for (std::size_t i = 0; i < xobjects.size(); ++i) {
        const std::string name = "Im" + std::to_string(i);
        pdf_dict_puts(ctx, xo, name.c_str(), xobjects[i]);
      }
    }
    fz_buffer* contents = fz_new_buffer_from_copied_data(
        ctx, reinterpret_cast<const unsigned char*>(content.data()), content.size());
    pdf_obj* pg = pdf_add_page(ctx, doc, fz_make_rect(0, 0, w, h), rotate, res, contents);
    pdf_insert_page(ctx, doc, -1, pg);
    pdf_drop_obj(ctx, pg);
    fz_drop_buffer(ctx, contents);
    pdf_drop_obj(ctx, res);
  }

  void square_annotation(int page_index) {
    pdf_page* p = pdf_load_page(ctx, doc, page_index);
    pdf_annot* a = pdf_create_annot(ctx, p, PDF_ANNOT_SQUARE);
    pdf_set_annot_rect(ctx, a, fz_make_rect(30, 30, 90, 70));
    const float red[3] = {1, 0, 0};
    pdf_set_annot_color(ctx, a, 3, red);
    pdf_update_annot(ctx, a);
    pdf_drop_annot(ctx, a);
    fz_drop_page(ctx, &p->super);
  }

  void save(const std::filesystem::path& out) {
    pdf_write_options opts = pdf_default_write_options;
    opts.do_compress = 1;
    pdf_save_document(ctx, doc, out.string().c_str(), &opts);
  }

 private:
  pdf_obj* font_ref = nullptr;
};

std::string full_page_image(int i) {
  char buf[96];
  std::snprintf(buf, sizeof buf, "q %g 0 0 %g 0 0 cm /Im%d Do Q\n", kPagePt, kPagePt, i);
  return buf;
}

const char* kText =
    "BT /F1 9 Tf 10 120 Td (The quick brown fox) Tj 0 -12 Td (jumps over the) Tj "
    "0 -12 Td (lazy dog 0123456789) Tj ET\n";

/// Runs @p body with a fresh context and document; returns an error message.
std::string with_document(const std::filesystem::path& out,
                          const std::function<void(Builder&)>& body) {
  fz_context* ctx = fz_new_context(nullptr, nullptr, FZ_STORE_DEFAULT);
  if (!ctx) return "fz_new_context failed";
  std::string error;
  Builder b;
  b.ctx = ctx;
  fz_try(ctx) {
    b.doc = pdf_create_document(ctx);
    body(b);
    b.save(out);
  }
  fz_always(ctx) {
    for (pdf_obj* o : b.keep) pdf_drop_obj(ctx, o);
    pdf_drop_document(ctx, b.doc);
  }
  fz_catch(ctx) { error = fz_caught_message(ctx); }
  fz_drop_context(ctx);
  return error;
}

}  // namespace

std::vector<PdfFixture> pdf_fixtures() {
  using K = PageContentKind;
  return {
      {"empty", K::Empty, 0, std::nullopt},
      {"vector-text", K::Vector, 0, std::nullopt},
      {"vector-diagram", K::Vector, 0, std::nullopt},
      {"scan-gray-300", K::Raster, 300, -1},
      {"scan-rgb-600", K::Raster, 600, -2},
      {"scan-ocr-300", K::Raster, 300, -1, false, true},
      {"scan-white-background", K::Raster, 300, -1, true},
      {"scan-stencil-400", K::Raster, 400, -2},
      {"mixed-raster-150-600", K::Raster, 600, -2},
      {"scan-bates-stamp", K::Mixed, 300, std::nullopt},
      {"scan-annotation", K::Mixed, 300, std::nullopt},
      {"clearscan", K::Mixed, 150, std::nullopt},
      {"photo-72", K::Raster, 72, 0},
      {"scan-rotated-300", K::Raster, 300, -1},
  };
}

std::string write_fixture_pdf(const std::filesystem::path& out) {
  return with_document(out, [](Builder& b) {
    // empty
    b.page("", {});
    // vector-text
    b.page(std::string(kText) + kText, {});
    // vector-diagram: strokes and a few glyphs (the old sparse-text rule
    // called this a scan).
    b.page("2 w 10 10 m 134 134 l S 10 134 m 134 10 l S 20 20 104 104 re S "
           "BT /F1 8 Tf 60 70 Td (A1) Tj ET\n",
           {});
    // scan-gray-300
    b.page(full_page_image(0), {b.image(600, 600, 1)});
    // scan-rgb-600
    b.page(full_page_image(0), {b.image(1200, 1200, 3)});
    // scan-ocr-300: invisible text (render mode 3) over the scan
    b.page(full_page_image(0) + "BT 3 Tr /F1 9 Tf 10 120 Td (OCR layer text) Tj ET\n",
           {b.image(600, 600, 1)});
    // scan-white-background: page-size white fill first
    b.page("1 g 0 0 144 144 re f\n" + full_page_image(0), {b.image(600, 600, 1)});
    // scan-stencil-400
    b.page("0 g " + full_page_image(0), {b.stencil(800, 800)});
    // mixed-raster-150-600: colour background + sharp bitonal text layer
    b.page(full_page_image(0) + "0 g " + full_page_image(1),
           {b.image(300, 300, 3), b.stencil(1200, 1200)});
    // scan-bates-stamp: visible vector text over a scan
    b.page(full_page_image(0) + "BT /F1 6 Tf 90 6 Td (ABC000123) Tj ET\n",
           {b.image(600, 600, 1)});
    // scan-annotation (annotation added below)
    b.page(full_page_image(0), {b.image(600, 600, 1)});
    // clearscan: low-res background + visible text glyphs
    b.page(full_page_image(0) + kText, {b.image(300, 300, 3)});
    // photo-72
    b.page(full_page_image(0), {b.image(144, 144, 3)});
    // scan-rotated-300: 2 x 3 in page, /Rotate 90
    b.page("q 144 0 0 216 0 0 cm /Im0 Do Q\n", {b.image(600, 900, 1)}, 144, 216, 90);

    b.square_annotation(10);
  });
}

std::string write_vector_letter_pdf(const std::filesystem::path& out) {
  return with_document(out, [](Builder& b) {
    std::string body = "0.5 w ";
    for (int i = 0; i <= 20; ++i) {
      body += std::to_string(36 + i * 27) + " 36 m " + std::to_string(36 + i * 27) +
              " 756 l S ";
    }
    for (int i = 0; i < 40; ++i) {
      body += "BT /F1 10 Tf 40 " + std::to_string(740 - i * 18) +
              " Td (Line " + std::to_string(i) + " ABCDEFGHIJKLMNOPQRSTUVWXYZ) Tj ET\n";
    }
    b.page(body, {}, 612, 792);
  });
}

}  // namespace thumtoo::fixtures
