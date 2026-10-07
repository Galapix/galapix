// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "thumtoo/constants.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace thumtoo {

/// Build a file:/// URI from an absolute filesystem path (DESIGN Location form).
[[nodiscard]] std::string file_uri_from_path(const std::filesystem::path& absolute_path);

/// If uri is file:///..., return the outer filesystem path (strips //archive and
/// //page pipes). Otherwise nullopt.
[[nodiscard]] std::optional<std::filesystem::path> path_from_file_uri(
    std::string_view uri);

/// True if uri uses the //archive: pipe form somewhere after file:///.
[[nodiscard]] bool is_archive_uri(std::string_view uri);

/// True if uri contains a PDF page pipe (//page:N, //mupdf-page:N; //poppler-page: accepted).
[[nodiscard]] bool is_pdf_page_uri(std::string_view uri);

/// True if uri contains //pdfimage:N (embedded image extract, 1-based).
[[nodiscard]] bool is_pdf_image_uri(std::string_view uri);

/// True if uri ends a collection expand directive //pdfimages (no index).
[[nodiscard]] bool is_pdf_images_collection_uri(std::string_view uri);

/// True if scheme is http: or https: (fetch not implemented yet).
[[nodiscard]] bool is_http_uri(std::string_view uri);

/// True if uri is a content-addressed id (sha256:… / sha1:…).
[[nodiscard]] bool is_content_id_uri(std::string_view uri);

/// Also see kContentIdSha256Prefix in constants.hpp.
inline constexpr std::string_view kContentIdSha1Prefix = "sha1:";

/// Build sha256:<hex> (hex may already include the prefix).
[[nodiscard]] std::string content_id_uri_from_sha256_hex(std::string_view hex);

/// Extract hex after sha256: / sha1: or nullopt.
[[nodiscard]] std::optional<std::string> content_id_hex(std::string_view uri);

// --- Composable Location model (DESIGN § Locator URI) ---

enum class UriScheme {
  File,
  Http,
  Https,
  ContentSha256,
  ContentSha1,
  Unknown,
};

enum class LocationPipeKind {
  ArchiveRoot,     // …//archive
  ArchiveMember,   // …//archive:member/path
  EpubLayout,      // …//epub:w=1200,h=1800,fs=12
  PdfPage,         // …//page:N (1-based, default PDF backend)
  PdfPagePoppler,  // deprecated; parse maps //poppler-page: → PdfPage
  PdfPageMupdf,    // …//mupdf-page:N
  PdfImage,        // …//pdfimage:N (1-based embedded image, native res)
  PdfImages,       // …//pdfimages (expand to all embedded images)
};

struct LocationPipe {
  LocationPipeKind kind = LocationPipeKind::ArchiveRoot;
  /// Member path, epub layout params, or decimal page number for PdfPage*;
  /// empty for ArchiveRoot.
  std::string value;
};

/// Parsed location: scheme + base + ordered pipes (outer → inner).
struct Location {
  UriScheme scheme = UriScheme::Unknown;
  /// Absolute path (file), host/path (http), or full content id string.
  std::string base;
  std::vector<LocationPipe> pipes;
};

/// Parse DESIGN Location form. Does not resolve or fetch.
[[nodiscard]] std::optional<Location> parse_location(std::string_view uri);

/// Format Location back to a URI string.
[[nodiscard]] std::string format_location(const Location& loc);

/// Append //archive:member (or //archive if member empty) onto a base URI.
[[nodiscard]] std::string with_archive_member(std::string_view base_uri,
                                             std::string_view member_path = {});

/// Append //page:N (1-based) — default PDF backend route.
[[nodiscard]] std::string with_pdf_page(std::string_view base_uri, int page_1based);

/// Append //pdfimage:N (1-based document-order embedded image).
[[nodiscard]] std::string with_pdf_image(std::string_view base_uri, int image_1based);

/// Append //pdfimages collection directive (expand via expand_pdf_image_uris).
[[nodiscard]] std::string with_pdf_images(std::string_view base_uri);

/// Deprecated alias of with_pdf_page (emits //page:N).
[[nodiscard]] std::string with_pdf_page_poppler(std::string_view base_uri,
                                                int page_1based);
[[nodiscard]] std::string with_pdf_page_mupdf(std::string_view base_uri,
                                              int page_1based);

/// Font family preset for //epub: ff=
enum class EpubFontFamily {
  Publisher = 0,  // do not force family (book fonts)
  Serif,
  Sans,
  Mono,
};

/// Colour theme preset for //epub: theme=
enum class EpubTheme {
  Day = 0,
  Sepia,
  Night,
};

/// Body text alignment for //epub: align=
enum class EpubAlign {
  Publisher = 0,  // do not force
  Left,
  Right,
  Center,
  Justify,
};

/// EPUB layout profile for //epub: URIs (reader policy; not part of the EPUB format).
/// w/h are pixels at kEpubLayoutDpi; fs is font size in points; margins in pixels.
/// lh / cols / cgap / align / ff / theme / pubcss → MuPDF user CSS (+ use_document_css).
struct EpubLayout {
  int width_px = 0;
  int height_px = 0;
  int fs_pt = 0;
  int mt_px = 0;
  int mr_px = 0;
  int mb_px = 0;
  int ml_px = 0;
  /// Line height as percent (140 = 1.4). Default 140; 0 = omit from CSS.
  int lh_percent = 0;
  /// CSS column-count (1 = single column / omit).
  int cols = 1;
  /// Column gap in pixels at kEpubLayoutDpi (0 = CSS default when cols > 1).
  int cgap_px = 0;
  EpubAlign align = EpubAlign::Publisher;
  EpubFontFamily font = EpubFontFamily::Publisher;
  EpubTheme theme = EpubTheme::Day;
  /// When false, MuPDF ignores publication CSS (pubcss=0).
  bool use_document_css = true;
};

[[nodiscard]] EpubLayout default_epub_layout();

/// Canonical //epub: payload: w,h,fs then optional non-default keys.
[[nodiscard]] std::string format_epub_layout_params(const EpubLayout& layout);

/// Parse layout keys from an //epub: value (missing keys keep defaults).
[[nodiscard]] EpubLayout parse_epub_layout_params(std::string_view params);

/// Append //epub:… layout payload onto a base URI.
[[nodiscard]] std::string with_epub_layout(std::string_view base_uri,
                                           const EpubLayout& layout);

/// True if uri contains an //epub: layout pipe.
[[nodiscard]] bool is_epub_layout_uri(std::string_view uri);

/// Region key for EPUB page tiles: "{page}|{format_epub_layout_params}".
/// Layout is part of identity (docs/EPUB.md); page-only keys collide across layouts.
[[nodiscard]] std::string epub_page_region_key(int page_1based, const EpubLayout& layout);
[[nodiscard]] std::string epub_page_region_key(int page_1based,
                                               std::string_view layout_key);

/// content_id suffix for an EPUB page: ":page:N:epub:{layout_key}".
[[nodiscard]] std::string epub_content_id_page_suffix(int page_1based,
                                                      const EpubLayout& layout);
[[nodiscard]] std::string epub_content_id_page_suffix(int page_1based,
                                                      std::string_view layout_key);

}  // namespace thumtoo
