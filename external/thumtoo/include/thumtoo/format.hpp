// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace thumtoo {

/// How prepare_paths / open classify a filesystem path (extension heuristic).
enum class PathKind {
  Unsupported = 0,
  Image,
  Archive,
  Pdf,
  Djvu,
  Epub,
  Markdown,  ///< MuPDF ≥ 1.28 native Markdown document
  PlainText, ///< MuPDF native plain text (.txt / .text)
};

/// Lowercased extension including the dot (".jpg"), or empty.
[[nodiscard]] std::string path_extension_lower(const std::filesystem::path& path);

/// Lowercased extension of a member basename (".png"), or empty.
[[nodiscard]] std::string member_extension_lower(std::string_view member_path);

[[nodiscard]] bool is_image_extension(std::string_view ext_with_dot);
[[nodiscard]] bool is_archive_filename(std::string_view lower_filename);
[[nodiscard]] bool is_pdf_extension(std::string_view ext_with_dot);
[[nodiscard]] bool is_djvu_extension(std::string_view ext_with_dot);
[[nodiscard]] bool is_epub_extension(std::string_view ext_with_dot);
[[nodiscard]] bool is_markdown_extension(std::string_view ext_with_dot);
[[nodiscard]] bool is_plain_text_extension(std::string_view ext_with_dot);

[[nodiscard]] bool is_image_path(const std::filesystem::path& path);
[[nodiscard]] bool is_archive_path(const std::filesystem::path& path);
[[nodiscard]] bool is_pdf_path(const std::filesystem::path& path);
[[nodiscard]] bool is_djvu_path(const std::filesystem::path& path);
[[nodiscard]] bool is_epub_path(const std::filesystem::path& path);
[[nodiscard]] bool is_markdown_path(const std::filesystem::path& path);
[[nodiscard]] bool is_plain_text_path(const std::filesystem::path& path);

/// PDF, Markdown, or plain text — multipage docs via MuPDF (//page:N).
[[nodiscard]] bool is_mupdf_page_document_path(const std::filesystem::path& path);

/// Native MuPDF text/md extensions (.txt/.text/.md/…) — open by filename.
[[nodiscard]] bool is_mupdf_native_text_extension(std::string_view ext_with_dot);

/// URI contains //text (optional //page:N) — force plain-text open via MuPDF.
[[nodiscard]] bool uri_has_text_force_pipe(std::string_view uri);

/// Strip //text (and optional following //page:… stays). Returns filesystem path
/// portion before any pipes; empty if not a text-force URI.
[[nodiscard]] std::optional<std::filesystem::path> path_from_text_force_uri(
    std::string_view uri);

[[nodiscard]] PathKind classify_path(const std::filesystem::path& path);

/// Extensions thumtoo will treat as raster images (including the dot).
[[nodiscard]] std::vector<std::string_view> image_extensions();

/// Archive container suffixes (including the dot; may be multi-dot like ".tar.gz").
[[nodiscard]] std::vector<std::string_view> archive_suffixes();

/// Freedesktop-style MIME types for images (+ PDF + common archives) for
/// .desktop files and file dialogs. Apps should use this instead of a private list.
[[nodiscard]] std::vector<std::string_view> media_mime_types();

/// Single line suitable for MimeType= in a .desktop file (semicolon-separated, trailing ;).
[[nodiscard]] std::string desktop_mime_types_line();

}  // namespace thumtoo
