// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace thumtoo {

/// Expand a filesystem path into one or more media location URIs for display.
///
/// - Raster image → single `file://…` URI
/// - Multipage PDF / Markdown / plain text / DjVu → `file://…//page:N` (1-based, capped)
/// - EPUB → `file://…//epub:w,h,em//page:N` under default layout (MuPDF)
/// - Archive (zip/cbz/rar/…) → `file://…//archive:member` for image members
/// - Missing / unreadable / non-media → empty vector (caller may keep original)
///
/// Does not touch the database; pure source inspection. Viewers should use this
/// instead of reimplementing format-specific page/TOC expansion.
[[nodiscard]] std::vector<std::string> expand_media_uris(
    const std::filesystem::path& path, int max_pages = 512);

/// True if the path is something expand_media_uris / prepare_paths would treat
/// as openable media (image, PDF, Markdown, plain text, DjVu, EPUB, or archive).
[[nodiscard]] bool is_openable_media_path(const std::filesystem::path& path);

/// Expand a PDF to `//pdfimage:1..N` (embedded Image XObjects at native res).
/// Empty if MuPDF unavailable or no images found. Does not change default
/// expand_media_uris (still uses rendered //page:N).
[[nodiscard]] std::vector<std::string> expand_pdf_image_uris(
    const std::filesystem::path& path, int max_images = 4096);

/// If @p uri is a //pdfimages collection (or bare PDF path policy), expand to
/// leaf locators. For `…//pdfimages` returns //pdfimage:1..N; otherwise empty
/// (caller should use expand_media_uris on the filesystem path).
[[nodiscard]] std::vector<std::string> expand_pdf_images_collection_uri(
    std::string_view uri, int max_images = 4096);

}  // namespace thumtoo
