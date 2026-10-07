// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "thumtoo/expand.hpp"

#include "thumtoo/archive.hpp"
#include "thumtoo/djvu.hpp"
#include "thumtoo/format.hpp"
#include "thumtoo/epub.hpp"
#include "thumtoo/pdf.hpp"
#include "thumtoo/pdf_mupdf.hpp"
#include "thumtoo/uri.hpp"

#include <algorithm>
#include <filesystem>
#include <string>

namespace thumtoo {

bool is_openable_media_path(const std::filesystem::path& path)
{
  std::filesystem::path use = path;
  const std::string raw = path.string();
  if (uri_has_text_force_pipe(raw)) {
    if (auto p = path_from_text_force_uri(raw)) {
      use = *p;
    }
  }
  switch (classify_path(use)) {
    case PathKind::Image:
    case PathKind::Pdf:
    case PathKind::Markdown:
    case PathKind::PlainText:
    case PathKind::Djvu:
    case PathKind::Epub:
    case PathKind::Archive:
      return true;
    case PathKind::Unsupported:
      return uri_has_text_force_pipe(raw);
  }
  return false;
}

namespace {

std::vector<std::string> expand_mupdf_page_uris(const std::filesystem::path& abs,
                                                int page_cap, bool emit_text_pipe)
{
  std::vector<std::string> out;
  if (emit_text_pipe || is_plain_text_path(abs)) {
    // Non-native text extensions need MuPDF filetype magic "txt".
    if (!is_mupdf_native_text_extension(path_extension_lower(abs))) {
      mupdf_force_next_open_as_text(abs);
    }
  }
  auto count = pdf_page_count(abs);
  if (!count || *count < 1) {
    out.push_back(file_uri_from_path(abs) + (emit_text_pipe ? "//text" : ""));
    return out;
  }
  const int n = std::min(*count, page_cap);
  out.reserve(static_cast<std::size_t>(n));
  for (int page = 1; page <= n; ++page) {
    if (emit_text_pipe) {
      out.push_back(file_uri_from_path(abs) + "//text//page:" + std::to_string(page));
    } else {
      out.push_back(pdf_page_uri(abs, page));
    }
  }
  return out;
}

}  // namespace

std::vector<std::string> expand_media_uris(const std::filesystem::path& path,
                                           int max_pages)
{
  std::vector<std::string> out;
  std::error_code ec;

  const std::string raw = path.string();
  const bool force_text = uri_has_text_force_pipe(raw);
  std::filesystem::path file_path = path;
  if (force_text) {
    if (auto p = path_from_text_force_uri(raw)) {
      file_path = *p;
    }
  }

  if (!std::filesystem::is_regular_file(file_path, ec) || ec) {
    return out;
  }

  const auto abs = file_path.lexically_normal();
  const int page_cap = max_pages > 0 ? max_pages : 512;

  if (force_text) {
    return expand_mupdf_page_uris(abs, page_cap, true);
  }

  switch (classify_path(abs)) {
    case PathKind::Pdf:
      return expand_mupdf_page_uris(abs, page_cap, false);
    case PathKind::Markdown:
    case PathKind::PlainText:
      return expand_mupdf_page_uris(abs, page_cap, false);
    case PathKind::Djvu: {
      auto count = djvu_page_count(abs);
      if (!count || *count < 1) {
        out.push_back(file_uri_from_path(abs));
        return out;
      }
      const int n = std::min(*count, page_cap);
      out.reserve(static_cast<std::size_t>(n));
      for (int page = 1; page <= n; ++page) {
        out.push_back(djvu_page_uri(abs, page));
      }
      return out;
    }
    case PathKind::Epub: {
      const auto layout = default_epub_layout();
      auto count = epub_page_count(abs, layout);
      if (!count || *count < 1) {
        out.push_back(file_uri_from_path(abs));
        return out;
      }
      const int n = std::min(*count, page_cap);
      out.reserve(static_cast<std::size_t>(n));
      for (int page = 1; page <= n; ++page) {
        out.push_back(epub_page_uri(abs, page, layout));
      }
      return out;
    }
    case PathKind::Archive: {
      auto toc = read_archive_toc(abs);
      if (!toc) {
        return out;
      }
      for (auto const& mem : *toc) {
        if (!is_likely_image_member_path(mem.member_path)) {
          continue;
        }
        out.push_back(archive_uri(abs, mem.member_path));
      }
      return out;
    }
    case PathKind::Image:
      out.push_back(file_uri_from_path(abs));
      return out;
    case PathKind::Unsupported:
      return out;
  }
  return out;
}

std::vector<std::string> expand_pdf_image_uris(const std::filesystem::path& path,
                                               int max_images)
{
  std::vector<std::string> out;
  std::error_code ec;
  if (!std::filesystem::is_regular_file(path, ec) || ec) {
    return out;
  }
  const auto abs = path.lexically_normal();
  auto count = pdf_embedded_image_count(abs);
  if (!count || *count < 1) {
    return out;
  }
  const int cap = max_images > 0 ? max_images : 4096;
  const int n = std::min(*count, cap);
  out.reserve(static_cast<std::size_t>(n));
  for (int i = 1; i <= n; ++i) {
    out.push_back(pdf_image_uri(abs, i));
  }
  return out;
}

std::vector<std::string> expand_pdf_images_collection_uri(std::string_view uri,
                                                          int max_images)
{
  if (!is_pdf_images_collection_uri(uri)) {
    return {};
  }
  auto path = path_from_file_uri(uri);
  if (!path) {
    auto pos = uri.find("//pdfimages");
    if (pos == std::string_view::npos) {
      return {};
    }
    path = std::filesystem::path(std::string(uri.substr(0, pos)));
  }
  return expand_pdf_image_uris(*path, max_images);
}

}  // namespace thumtoo
