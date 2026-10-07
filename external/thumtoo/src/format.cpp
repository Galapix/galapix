// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "thumtoo/format.hpp"
#include "thumtoo/uri.hpp"
#include <optional>

#include <cctype>
#include <string>

namespace thumtoo {
namespace {

void ascii_tolower_inplace(std::string& s) {
  for (char& c : s)
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
}

// Raster formats libvips (and our probe/ladder/tile paths) commonly handle.
constexpr std::string_view kImageExts[] = {
    ".jpg", ".jpeg", ".jpe", ".png", ".gif", ".bmp", ".webp", ".jxl",
    ".tif", ".tiff", ".heic", ".heif", ".avif",
};

// Multi-suffix archives compared against the full lowercased filename.
constexpr std::string_view kArchiveSuffixes[] = {
    ".zip",     ".cbz",     ".cbr",     ".rar",     ".7z",
    ".tar",     ".tgz",     ".tbz2",    ".txz",
    ".tar.gz",  ".tar.bz2", ".tar.xz",
};

constexpr std::string_view kMediaMimes[] = {
    // Images
    "image/jpeg",
    "image/png",
    "image/gif",
    "image/bmp",
    "image/webp",
    "image/jxl",
    "image/tiff",
    "image/heic",
    "image/heif",
    "image/avif",
    // Documents (page raster via MuPDF / DjVuLibre when enabled)
    "application/pdf",
    "image/vnd.djvu",
    "image/vnd.djvu+multipage",
    "application/epub+zip",
    "text/markdown",
    "text/x-markdown",
    "text/plain",
    // Archives (expanded to image members)
    "application/zip",
    "application/vnd.rar",
    "application/x-7z-compressed",
    "application/x-tar",
    "application/gzip",
};

}  // namespace

std::string path_extension_lower(const std::filesystem::path& path) {
  auto ext = path.extension().string();
  ascii_tolower_inplace(ext);
  return ext;
}

std::string member_extension_lower(std::string_view member_path) {
  if (member_path.empty()) return {};
  const auto slash = member_path.find_last_of("/\\");
  const auto name = slash == std::string_view::npos
                        ? member_path
                        : member_path.substr(slash + 1);
  const auto dot = name.find_last_of('.');
  if (dot == std::string_view::npos) return {};
  std::string ext(name.substr(dot));
  ascii_tolower_inplace(ext);
  return ext;
}

bool is_image_extension(std::string_view ext_with_dot) {
  for (auto e : kImageExts) {
    if (ext_with_dot == e) return true;
  }
  return false;
}

bool is_archive_filename(std::string_view lower_filename) {
  for (auto suf : kArchiveSuffixes) {
    if (lower_filename.size() >= suf.size() &&
        lower_filename.compare(lower_filename.size() - suf.size(), suf.size(),
                               suf) == 0)
      return true;
  }
  return false;
}

bool is_pdf_extension(std::string_view ext_with_dot) {
  return ext_with_dot == ".pdf";
}

bool is_epub_extension(std::string_view ext_with_dot) {
  return ext_with_dot == ".epub";
}

bool is_markdown_extension(std::string_view ext_with_dot) {
  return ext_with_dot == ".md" || ext_with_dot == ".markdown" ||
         ext_with_dot == ".mdown" || ext_with_dot == ".mkd";
}

bool is_plain_text_extension(std::string_view ext_with_dot) {
  // .txt/.text: MuPDF native. Others: open as text via filetype magic "txt".
  static constexpr std::string_view kExts[] = {
      ".txt", ".text",
      // sources / headers
      ".c", ".h", ".cc", ".hh", ".cpp", ".cxx", ".hpp", ".hxx", ".ipp", ".tcc",
      ".m", ".mm", ".swift", ".go", ".rs", ".java", ".kt", ".kts", ".scala",
      ".cs", ".fs", ".fsx", ".vb",
      ".py", ".pyi", ".pyw", ".rb", ".pl", ".pm", ".php", ".lua", ".r", ".jl",
      ".js", ".mjs", ".cjs", ".ts", ".tsx", ".jsx", ".vue", ".svelte",
      ".sh", ".bash", ".zsh", ".fish", ".ps1", ".bat", ".cmd",
      ".sql", ".graphql", ".gql",
      // markup / data (not Markdown — that is PathKind::Markdown)
      ".html", ".htm", ".xhtml", ".xml", ".xsl", ".xsd", ".svg",
      ".json", ".jsonc", ".json5", ".yaml", ".yml", ".toml", ".ini", ".cfg",
      ".conf", ".config", ".properties", ".env", ".desktop",
      ".csv", ".tsv", ".log", ".out", ".err",
      ".cmake", ".make", ".mak", ".ninja", ".gradle", ".sbt",
      ".dockerfile", ".containerfile",
      ".diff", ".patch",
      ".tex", ".ltx", ".sty", ".cls",
      ".rst", ".adoc", ".org", ".wiki",
      ".css", ".scss", ".sass", ".less",
      ".proto", ".thrift", ".idl",
  };
  for (auto e : kExts) {
    if (ext_with_dot == e) return true;
  }
  return false;
}

bool is_mupdf_native_text_extension(std::string_view ext_with_dot) {
  return ext_with_dot == ".txt" || ext_with_dot == ".text" ||
         is_markdown_extension(ext_with_dot);
}

bool is_djvu_extension(std::string_view ext_with_dot) {
  return ext_with_dot == ".djvu" || ext_with_dot == ".djv";
}

bool is_image_path(const std::filesystem::path& path) {
  return is_image_extension(path_extension_lower(path));
}

bool is_archive_path(const std::filesystem::path& path) {
  std::string name = path.filename().string();
  ascii_tolower_inplace(name);
  return is_archive_filename(name);
}

bool is_pdf_path(const std::filesystem::path& path) {
  return is_pdf_extension(path_extension_lower(path));
}

bool is_djvu_path(const std::filesystem::path& path) {
  return is_djvu_extension(path_extension_lower(path));
}

bool is_epub_path(const std::filesystem::path& path) {
  return is_epub_extension(path_extension_lower(path));
}

bool is_markdown_path(const std::filesystem::path& path) {
  return is_markdown_extension(path_extension_lower(path));
}

bool is_plain_text_path(const std::filesystem::path& path) {
  return is_plain_text_extension(path_extension_lower(path));
}

bool is_mupdf_page_document_path(const std::filesystem::path& path) {
  return is_pdf_path(path) || is_markdown_path(path) || is_plain_text_path(path);
}

PathKind classify_path(const std::filesystem::path& path) {
  // Order: archive/document before generic image.
  // Note: .epub is a zip; classify as Epub before Archive so we layout pages
  // instead of expanding as a zip of HTML/CSS members.
  if (is_epub_path(path)) return PathKind::Epub;
  if (is_archive_path(path)) return PathKind::Archive;
  if (is_pdf_path(path)) return PathKind::Pdf;
  if (is_markdown_path(path)) return PathKind::Markdown;
  if (is_plain_text_path(path)) return PathKind::PlainText;
  if (is_djvu_path(path)) return PathKind::Djvu;
  if (is_image_path(path)) return PathKind::Image;
  return PathKind::Unsupported;
}

std::vector<std::string_view> image_extensions() {
  return {std::begin(kImageExts), std::end(kImageExts)};
}

std::vector<std::string_view> archive_suffixes() {
  return {std::begin(kArchiveSuffixes), std::end(kArchiveSuffixes)};
}

std::vector<std::string_view> media_mime_types() {
  return {std::begin(kMediaMimes), std::end(kMediaMimes)};
}

std::string desktop_mime_types_line() {
  std::string out;
  for (auto m : kMediaMimes) {
    out.append(m);
    out.push_back(';');
  }
  return out;
}

bool uri_has_text_force_pipe(std::string_view uri) {
  // Match //text as a pipe segment (//text or //text//page:…).
  constexpr std::string_view k = "//text";
  auto pos = uri.find(k);
  while (pos != std::string_view::npos) {
    const auto after = pos + k.size();
    if (after >= uri.size() || uri[after] == '/' || uri[after] == '?' ||
        uri[after] == '#') {
      return true;
    }
    // //textsomething — not our pipe
    pos = uri.find(k, pos + 1);
  }
  return false;
}

std::optional<std::filesystem::path> path_from_text_force_uri(std::string_view uri) {
  if (!uri_has_text_force_pipe(uri)) return std::nullopt;
  constexpr std::string_view k = "//text";
  auto pos = uri.find(k);
  if (pos == std::string_view::npos) return std::nullopt;
  std::string_view left = uri.substr(0, pos);
  if (left.starts_with("file://")) {
    return path_from_file_uri(left);
  }
  if (!left.empty() && left.front() == '/') {
    return std::filesystem::path(std::string(left));
  }
  // relative / bare path
  if (!left.empty()) {
    return std::filesystem::path(std::string(left));
  }
  return std::nullopt;
}

}  // namespace thumtoo
