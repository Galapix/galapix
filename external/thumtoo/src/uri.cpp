// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "thumtoo/uri.hpp"

#include <cctype>
#include <sstream>

namespace thumtoo {
namespace {

constexpr std::string_view kArchivePipe = "//archive";
constexpr std::string_view kEpubPipe = "//epub:";
constexpr std::string_view kPagePipe = "//page:";
constexpr std::string_view kPopplerPagePipe = "//poppler-page:";
constexpr std::string_view kMupdfPagePipe = "//mupdf-page:";
constexpr std::string_view kPdfImagePipe = "//pdfimage:";
constexpr std::string_view kPdfImagesPipe = "//pdfimages";

enum class PipeHit {
  Archive,
  EpubLayout,
  PdfPage,
  PdfPagePoppler,
  PdfPageMupdf,
  PdfImage,
  PdfImages,
};

[[nodiscard]] std::size_t find_first_pipe(std::string_view rest, std::size_t from,
                                          PipeHit* hit) {
  const auto arch = rest.find(kArchivePipe, from);
  const auto epub = rest.find(kEpubPipe, from);
  const auto page = rest.find(kPagePipe, from);
  const auto pop = rest.find(kPopplerPagePipe, from);
  const auto mu = rest.find(kMupdfPagePipe, from);
  const auto pimgs = rest.find(kPdfImagesPipe, from);
  const auto pimg = rest.find(kPdfImagePipe, from);
  std::size_t next = std::string_view::npos;
  auto consider = [&](std::size_t pos, PipeHit kind) {
    if (pos == std::string_view::npos) return;
    if (next == std::string_view::npos || pos < next) {
      next = pos;
      *hit = kind;
    }
  };
  consider(arch, PipeHit::Archive);
  consider(epub, PipeHit::EpubLayout);
  consider(page, PipeHit::PdfPage);
  consider(pop, PipeHit::PdfPagePoppler);
  consider(mu, PipeHit::PdfPageMupdf);
  // //pdfimages before //pdfimage: (distinct strings; order is explicit).
  consider(pimgs, PipeHit::PdfImages);
  consider(pimg, PipeHit::PdfImage);
  return next;
}

std::string percent_decode_path(std::string_view rest) {
  std::string path;
  path.reserve(rest.size());
  for (std::size_t i = 0; i < rest.size(); ++i) {
    if (rest[i] == '%' && i + 2 < rest.size()) {
      auto hex = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
      };
      const int hi = hex(rest[i + 1]);
      const int lo = hex(rest[i + 2]);
      if (hi >= 0 && lo >= 0) {
        path.push_back(static_cast<char>((hi << 4) | lo));
        i += 2;
        continue;
      }
    }
    path.push_back(rest[i]);
  }
  return path;
}

std::string_view strip_pipes(std::string_view rest) {
  PipeHit hit = PipeHit::Archive;
  const auto next = find_first_pipe(rest, 0, &hit);
  if (next == std::string_view::npos) return rest;
  return rest.substr(0, next);
}

bool parse_pipes(std::string_view rest, std::vector<LocationPipe>& out) {
  std::size_t i = 0;
  while (i < rest.size()) {
    PipeHit hit = PipeHit::Archive;
    const std::size_t next = find_first_pipe(rest, i, &hit);
    if (next == std::string_view::npos) break;

    if (hit == PipeHit::PdfPage || hit == PipeHit::PdfPagePoppler ||
        hit == PipeHit::PdfPageMupdf) {
      std::string_view tag = kPagePipe;
      LocationPipeKind page_kind = LocationPipeKind::PdfPage;
      if (hit == PipeHit::PdfPagePoppler) {
        // Legacy //poppler-page: — parse as default PdfPage (MuPDF).
        tag = kPopplerPagePipe;
        page_kind = LocationPipeKind::PdfPage;
      } else if (hit == PipeHit::PdfPageMupdf) {
        tag = kMupdfPagePipe;
        page_kind = LocationPipeKind::PdfPageMupdf;
      }
      std::string_view after = rest.substr(next + tag.size());
      std::size_t n = 0;
      while (n < after.size() && after[n] >= '0' && after[n] <= '9') ++n;
      if (n == 0) return false;
      LocationPipe pipe;
      pipe.kind = page_kind;
      pipe.value = std::string(after.substr(0, n));
      out.push_back(std::move(pipe));
      i = next + tag.size() + n;
    } else if (hit == PipeHit::PdfImages) {
      // Collection directive — no index; value empty.
      LocationPipe pipe;
      pipe.kind = LocationPipeKind::PdfImages;
      pipe.value.clear();
      out.push_back(std::move(pipe));
      i = next + kPdfImagesPipe.size();
    } else if (hit == PipeHit::PdfImage) {
      std::string_view after = rest.substr(next + kPdfImagePipe.size());
      std::size_t n = 0;
      while (n < after.size() && after[n] >= '0' && after[n] <= '9') ++n;
      if (n == 0) return false;
      LocationPipe pipe;
      pipe.kind = LocationPipeKind::PdfImage;
      pipe.value = std::string(after.substr(0, n));
      out.push_back(std::move(pipe));
      i = next + kPdfImagePipe.size() + n;
    } else if (hit == PipeHit::EpubLayout) {
      std::string_view after = rest.substr(next + kEpubPipe.size());
      std::size_t end = after.size();
      PipeHit dummy = PipeHit::Archive;
      const auto np = find_first_pipe(after, 0, &dummy);
      if (np != std::string_view::npos) end = np;
      LocationPipe pipe;
      pipe.kind = LocationPipeKind::EpubLayout;
      // Canonical key order so hand-written URIs share cache keys with format().
      pipe.value = format_epub_layout_params(
          parse_epub_layout_params(after.substr(0, end)));
      out.push_back(std::move(pipe));
      i = next + kEpubPipe.size() + end;
    } else {
      std::string_view after = rest.substr(next + kArchivePipe.size());
      LocationPipe pipe;
      if (after.empty()) {
        pipe.kind = LocationPipeKind::ArchiveRoot;
        pipe.value.clear();
        out.push_back(std::move(pipe));
        i = rest.size();
        break;
      }
      if (after.front() != ':') {
        return false;
      }
      after.remove_prefix(1);
      std::size_t mem_end = after.size();
      PipeHit dummy = PipeHit::Archive;
      const auto np = find_first_pipe(after, 0, &dummy);
      if (np != std::string_view::npos) mem_end = np;
      pipe.kind = LocationPipeKind::ArchiveMember;
      pipe.value = std::string(after.substr(0, mem_end));
      out.push_back(std::move(pipe));
      i = next + kArchivePipe.size() + 1 + mem_end;
    }
  }
  return true;
}

}  // namespace

std::string file_uri_from_path(const std::filesystem::path& absolute_path) {
  const auto p = absolute_path.lexically_normal().generic_string();
  std::string out = "file://";
  for (char c : p) {
    if (c == ' ')
      out += "%20";
    else
      out += c;
  }
  return out;
}

std::optional<std::filesystem::path> path_from_file_uri(std::string_view uri) {
  constexpr std::string_view kPrefix = "file://";
  if (!uri.starts_with(kPrefix)) return std::nullopt;
  std::string_view rest = uri.substr(kPrefix.size());
  rest = strip_pipes(rest);
  std::string path = percent_decode_path(rest);
  if (path.empty()) return std::nullopt;
  return std::filesystem::path(path);
}

bool is_archive_uri(std::string_view uri) {
  return uri.find(kArchivePipe) != std::string_view::npos;
}

bool is_pdf_page_uri(std::string_view uri) {
  return uri.find(kPagePipe) != std::string_view::npos ||
         uri.find(kPopplerPagePipe) != std::string_view::npos ||
         uri.find(kMupdfPagePipe) != std::string_view::npos;
}

bool is_pdf_image_uri(std::string_view uri) {
  return uri.find(kPdfImagePipe) != std::string_view::npos;
}

bool is_pdf_images_collection_uri(std::string_view uri) {
  return uri.find(kPdfImagesPipe) != std::string_view::npos;
}

bool is_http_uri(std::string_view uri) {
  return uri.starts_with("http://") || uri.starts_with("https://");
}

bool is_content_id_uri(std::string_view uri) {
  return uri.starts_with(kContentIdSha256Prefix) || uri.starts_with(kContentIdSha1Prefix);
}

std::string content_id_uri_from_sha256_hex(std::string_view hex) {
  if (hex.starts_with(kContentIdSha256Prefix)) return std::string(hex);
  std::string out(kContentIdSha256Prefix);
  out.append(hex);
  return out;
}

std::optional<std::string> content_id_hex(std::string_view uri) {
  if (uri.starts_with(kContentIdSha256Prefix))
    return std::string(uri.substr(kContentIdSha256Prefix.size()));
  if (uri.starts_with(kContentIdSha1Prefix))
    return std::string(uri.substr(kContentIdSha1Prefix.size()));
  return std::nullopt;
}

std::optional<Location> parse_location(std::string_view uri) {
  Location loc;
  if (uri.starts_with("file://")) {
    loc.scheme = UriScheme::File;
    std::string_view rest = uri.substr(7);
    auto base = strip_pipes(rest);
    loc.base = percent_decode_path(base);
    if (loc.base.empty()) return std::nullopt;
    if (!parse_pipes(rest, loc.pipes)) return std::nullopt;
    return loc;
  }
  if (uri.starts_with("https://")) {
    loc.scheme = UriScheme::Https;
    loc.base = std::string(uri.substr(8));
    // Pipes on remote URLs are reserved for later; reject nested for now.
    if (loc.base.find("//archive") != std::string::npos ||
        loc.base.find("//page:") != std::string::npos ||
        loc.base.find("//poppler-page:") != std::string::npos ||
        loc.base.find("//mupdf-page:") != std::string::npos) {
      // Allow pipes on the path portion for future networked archives
      std::string_view rest = uri.substr(8);
      auto base = strip_pipes(rest);
      loc.base = std::string(base);
      if (!parse_pipes(rest, loc.pipes)) return std::nullopt;
    }
    if (loc.base.empty()) return std::nullopt;
    return loc;
  }
  if (uri.starts_with("http://")) {
    loc.scheme = UriScheme::Http;
    std::string_view rest = uri.substr(7);
    auto base = strip_pipes(rest);
    loc.base = std::string(base);
    if (loc.base.empty()) return std::nullopt;
    if (!parse_pipes(rest, loc.pipes)) return std::nullopt;
    return loc;
  }
  if (uri.starts_with(kContentIdSha256Prefix)) {
    loc.scheme = UriScheme::ContentSha256;
    loc.base = std::string(uri);
    return loc;
  }
  if (uri.starts_with(kContentIdSha1Prefix)) {
    loc.scheme = UriScheme::ContentSha1;
    loc.base = std::string(uri);
    return loc;
  }
  return std::nullopt;
}

std::string format_location(const Location& loc) {
  std::string out;
  switch (loc.scheme) {
    case UriScheme::File:
      out = file_uri_from_path(loc.base);
      break;
    case UriScheme::Http:
      out = "http://";
      out += loc.base;
      break;
    case UriScheme::Https:
      out = "https://";
      out += loc.base;
      break;
    case UriScheme::ContentSha256:
    case UriScheme::ContentSha1:
      return loc.base;
    case UriScheme::Unknown:
      return {};
  }
  for (const auto& pipe : loc.pipes) {
    switch (pipe.kind) {
      case LocationPipeKind::ArchiveRoot:
        out += "//archive";
        break;
      case LocationPipeKind::ArchiveMember:
        out += "//archive:";
        out += pipe.value;
        break;
      case LocationPipeKind::PdfPage:
        out += "//page:";
        out += pipe.value;
        break;
      case LocationPipeKind::PdfPagePoppler:
        // Legacy kind: never re-emit //poppler-page:.
        out += "//page:";
        out += pipe.value;
        break;
      case LocationPipeKind::PdfPageMupdf:
        out += "//mupdf-page:";
        out += pipe.value;
        break;
      case LocationPipeKind::EpubLayout:
        out += "//epub:";
        out += format_epub_layout_params(parse_epub_layout_params(pipe.value));
        break;
      case LocationPipeKind::PdfImage:
        out += "//pdfimage:";
        out += pipe.value;
        break;
      case LocationPipeKind::PdfImages:
        out += "//pdfimages";
        break;
    }
  }
  return out;
}

std::string with_archive_member(std::string_view base_uri, std::string_view member_path) {
  std::string out(base_uri);
  if (member_path.empty()) {
    out += "//archive";
  } else {
    out += "//archive:";
    out.append(member_path);
  }
  return out;
}

std::string with_pdf_page(std::string_view base_uri, int page_1based) {
  if (page_1based < 1) page_1based = 1;
  std::string out(base_uri);
  out += "//page:";
  out += std::to_string(page_1based);
  return out;
}

std::string with_pdf_page_poppler(std::string_view base_uri, int page_1based) {
  // Legacy API: emit neutral //page:N (MuPDF). Name kept for binary/source
  // compatibility; //poppler-page: is no longer written for new URIs.
  if (page_1based < 1) page_1based = 1;
  std::string out(base_uri);
  out += "//page:";
  out += std::to_string(page_1based);
  return out;
}

std::string with_pdf_page_mupdf(std::string_view base_uri, int page_1based) {
  if (page_1based < 1) page_1based = 1;
  std::string out(base_uri);
  out += "//mupdf-page:";
  out += std::to_string(page_1based);
  return out;
}

std::string with_pdf_image(std::string_view base_uri, int image_1based) {
  if (image_1based < 1) image_1based = 1;
  std::string out(base_uri);
  out += "//pdfimage:";
  out += std::to_string(image_1based);
  return out;
}

std::string with_pdf_images(std::string_view base_uri) {
  std::string out(base_uri);
  out += "//pdfimages";
  return out;
}

EpubLayout default_epub_layout() {
  EpubLayout L;
  L.width_px = kEpubDefaultPageWidthPx;
  L.height_px = kEpubDefaultPageHeightPx;
  L.fs_pt = kEpubDefaultFontSizePt;
  L.lh_percent = kEpubDefaultLineHeightPercent;
  L.cols = 1;
  return L;
}

namespace {

const char* font_family_token(EpubFontFamily f) {
  switch (f) {
    case EpubFontFamily::Serif: return "serif";
    case EpubFontFamily::Sans: return "sans";
    case EpubFontFamily::Mono: return "mono";
    case EpubFontFamily::Publisher:
    default: return "publisher";
  }
}

const char* theme_token(EpubTheme t) {
  switch (t) {
    case EpubTheme::Sepia: return "sepia";
    case EpubTheme::Night: return "night";
    case EpubTheme::Day:
    default: return "day";
  }
}

EpubFontFamily parse_font_family(std::string_view v) {
  if (v == "serif") return EpubFontFamily::Serif;
  if (v == "sans" || v == "sans-serif") return EpubFontFamily::Sans;
  if (v == "mono" || v == "monospace") return EpubFontFamily::Mono;
  return EpubFontFamily::Publisher;
}

EpubTheme parse_theme(std::string_view v) {
  if (v == "sepia") return EpubTheme::Sepia;
  if (v == "night" || v == "dark") return EpubTheme::Night;
  return EpubTheme::Day;
}

const char* align_token(EpubAlign a) {
  switch (a) {
    case EpubAlign::Left: return "left";
    case EpubAlign::Right: return "right";
    case EpubAlign::Center: return "center";
    case EpubAlign::Justify: return "justify";
    case EpubAlign::Publisher:
    default: return "publisher";
  }
}

EpubAlign parse_align(std::string_view v) {
  if (v == "left") return EpubAlign::Left;
  if (v == "right") return EpubAlign::Right;
  if (v == "center" || v == "centre") return EpubAlign::Center;
  if (v == "justify") return EpubAlign::Justify;
  return EpubAlign::Publisher;
}

}  // namespace

std::string format_epub_layout_params(const EpubLayout& layout) {
  // Canonical order: w,h,fs[,mt,mr,mb,ml][,lh][,ff][,theme][,pubcss]
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
  if (L.cgap_px < 0) L.cgap_px = 0;
  std::string out = "w=";
  out += std::to_string(L.width_px);
  out += ",h=";
  out += std::to_string(L.height_px);
  out += ",fs=";
  out += std::to_string(L.fs_pt);
  if (L.mt_px > 0 || L.mr_px > 0 || L.mb_px > 0 || L.ml_px > 0) {
    out += ",mt=";
    out += std::to_string(L.mt_px);
    out += ",mr=";
    out += std::to_string(L.mr_px);
    out += ",mb=";
    out += std::to_string(L.mb_px);
    out += ",ml=";
    out += std::to_string(L.ml_px);
  }
  // Always emit lh when non-zero (default profile includes 140).
  if (L.lh_percent > 0) {
    out += ",lh=";
    out += std::to_string(L.lh_percent);
  }
  if (L.cols > 1) {
    out += ",cols=";
    out += std::to_string(L.cols);
    if (L.cgap_px > 0) {
      out += ",cgap=";
      out += std::to_string(L.cgap_px);
    }
  }
  if (L.align != EpubAlign::Publisher) {
    out += ",align=";
    out += align_token(L.align);
  }
  if (L.font != EpubFontFamily::Publisher) {
    out += ",ff=";
    out += font_family_token(L.font);
  }
  if (L.theme != EpubTheme::Day) {
    out += ",theme=";
    out += theme_token(L.theme);
  }
  if (!L.use_document_css) {
    out += ",pubcss=0";
  }
  return out;
}

EpubLayout parse_epub_layout_params(std::string_view params) {
  EpubLayout L = default_epub_layout();
  std::size_t i = 0;
  while (i < params.size()) {
    while (i < params.size() && (params[i] == ',' || params[i] == ' ')) ++i;
    if (i >= params.size()) break;
    std::size_t eq = params.find('=', i);
    if (eq == std::string_view::npos) break;
    std::string_view key = params.substr(i, eq - i);
    std::size_t vstart = eq + 1;
    std::size_t vend = params.find(',', vstart);
    if (vend == std::string_view::npos) vend = params.size();
    std::string_view val = params.substr(vstart, vend - vstart);
    // Trim spaces in value
    while (!val.empty() && val.front() == ' ') val.remove_prefix(1);
    while (!val.empty() && val.back() == ' ') val.remove_suffix(1);

    auto as_int = [&](int* out) -> bool {
      if (val.empty()) return false;
      int n = 0;
      for (char c : val) {
        if (c < '0' || c > '9') return false;
        n = n * 10 + (c - '0');
      }
      *out = n;
      return true;
    };

    int n = 0;
    if (key == "w" && as_int(&n) && n >= 1) {
      L.width_px = n;
    } else if (key == "h" && as_int(&n) && n >= 1) {
      L.height_px = n;
    } else if (key == "fs" && as_int(&n) && n >= 1) {
      L.fs_pt = n;
    } else if (key == "mt" && as_int(&n) && n >= 0) {
      L.mt_px = n;
    } else if (key == "mr" && as_int(&n) && n >= 0) {
      L.mr_px = n;
    } else if (key == "mb" && as_int(&n) && n >= 0) {
      L.mb_px = n;
    } else if (key == "ml" && as_int(&n) && n >= 0) {
      L.ml_px = n;
    } else if (key == "lh" && as_int(&n) && n >= 50 && n <= 400) {
      L.lh_percent = n;
    } else if ((key == "cols" || key == "columns") && as_int(&n) && n >= 1 && n <= 6) {
      L.cols = n;
    } else if ((key == "cgap" || key == "colgap") && as_int(&n) && n >= 0) {
      L.cgap_px = n;
    } else if (key == "align") {
      L.align = parse_align(val);
    } else if (key == "ff") {
      L.font = parse_font_family(val);
    } else if (key == "theme") {
      L.theme = parse_theme(val);
    } else if (key == "pubcss" && as_int(&n)) {
      L.use_document_css = (n != 0);
    }
    i = vend;
  }
  return L;
}

std::string with_epub_layout(std::string_view base_uri, const EpubLayout& layout) {
  std::string out(base_uri);
  out += "//epub:";
  out += format_epub_layout_params(layout);
  return out;
}

bool is_epub_layout_uri(std::string_view uri) {
  return uri.find(kEpubPipe) != std::string_view::npos;
}

std::string epub_page_region_key(int page_1based, std::string_view layout_key) {
  std::string key = std::to_string(page_1based);
  if (!layout_key.empty()) {
    key.push_back('|');
    key.append(layout_key);
  }
  return key;
}

std::string epub_page_region_key(int page_1based, const EpubLayout& layout) {
  return epub_page_region_key(page_1based, format_epub_layout_params(layout));
}

std::string epub_content_id_page_suffix(int page_1based, std::string_view layout_key) {
  std::string out = ":page:";
  out += std::to_string(page_1based);
  if (!layout_key.empty()) {
    out += ":epub:";
    out.append(layout_key);
  }
  return out;
}

std::string epub_content_id_page_suffix(int page_1based, const EpubLayout& layout) {
  return epub_content_id_page_suffix(page_1based, format_epub_layout_params(layout));
}

}  // namespace thumtoo
