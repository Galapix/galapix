// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/// Optional libunarr backend for RAR/CBR (including solid RAR that libarchive
/// cannot extract). Sequential entry walk only — solid dictionaries do not
/// support free random member payload seeks.

#include "thumtoo/archive.hpp"
#include "thumtoo/constants.hpp"
#include "thumtoo/build_stats.hpp"
#include "thumtoo/debug.hpp"

#include <unarr.h>

#include <cctype>
#include <chrono>
#include <cstdio>
#include <memory>
#include <string>

namespace thumtoo {
namespace {

bool member_paths_equal(std::string_view a, std::string_view b) {
  auto strip = [](std::string_view s) {
    while (s.starts_with("./")) s.remove_prefix(2);
    return s;
  };
  a = strip(a);
  b = strip(b);
  if (a == b) return true;
  std::string aa(a), bb(b);
  for (char& c : aa) {
    if (c == '\\') c = '/';
    else c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  }
  for (char& c : bb) {
    if (c == '\\') c = '/';
    else c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  }
  return aa == bb;
}

bool path_looks_rar(const std::filesystem::path& path) {
  std::string ext = path.extension().string();
  for (char& c : ext)
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  if (ext == ".rar" || ext == ".cbr") return true;
  std::string lower = path.filename().string();
  for (char& c : lower)
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return lower.find(".rar") != std::string::npos;
}

// RAR magic probe (visible to archive_is_rar5 / prefers after this TU section).
bool file_is_rar5(const std::filesystem::path& path) {
  std::FILE* f = std::fopen(path.string().c_str(), "rb");
  if (!f) return false;
  unsigned char mag[8] = {};
  const size_t n = std::fread(mag, 1, 8, f);
  std::fclose(f);
  if (n < 7) return false;
  return mag[0] == 'R' && mag[1] == 'a' && mag[2] == 'r' && mag[3] == '!'
         && mag[4] == 0x1a && mag[5] == 0x07 && mag[6] == 0x01;
}

struct UnarrHolder {
  ar_stream* stream = nullptr;
  ar_archive* ar = nullptr;
  std::string path_for_log;
  ~UnarrHolder() {
    if (ar || stream) {
      THUMTOO_ARCHIVE_DBG("unarr close path=%s", path_for_log.c_str());
    }
    if (ar) ar_close_archive(ar);
    if (stream) ar_close(stream);
  }
};

std::unique_ptr<UnarrHolder> open_rar(const std::filesystem::path& path) {
  if (file_is_rar5(path)) {
    THUMTOO_ARCHIVE_DBG("unarr open skip RAR5 path=%s", path.string().c_str());
    return nullptr;
  }
  const auto t0 = std::chrono::steady_clock::now();
  auto u = std::make_unique<UnarrHolder>();
  u->path_for_log = path.string();
  u->stream = ar_open_file(path.string().c_str());
  if (!u->stream) {
    THUMTOO_ARCHIVE_DBG("unarr open file FAILED path=%s", path.string().c_str());
    return nullptr;
  }
  u->ar = ar_open_rar_archive(u->stream);
  if (!u->ar) {
    THUMTOO_ARCHIVE_DBG("unarr open rar FAILED path=%s", path.string().c_str());
    return nullptr;
  }
  const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::steady_clock::now() - t0)
                      .count();
  THUMTOO_ARCHIVE_DBG("unarr open ok ms=%lld path=%s",
                      static_cast<long long>(ms), path.string().c_str());
  return u;
}

std::optional<std::vector<std::uint8_t>> uncompress_current(ar_archive* ar) {
  const size_t declared = ar_entry_get_size(ar);
  if (declared > kArchiveMaxMemberUncompressedBytes) return std::nullopt;
  std::vector<std::uint8_t> buf(declared);
  if (declared == 0) return buf;
  if (!ar_entry_uncompress(ar, buf.data(), declared)) return std::nullopt;
  return buf;
}

}  // namespace

bool unarr_backend_available() { return true; }

bool archive_is_rar5(const std::filesystem::path& archive_path) {
  return file_is_rar5(archive_path);
}

bool archive_prefers_unarr(const std::filesystem::path& archive_path) {
  // RAR5: never call unarr (stderr spam + null). Libarchive for non-solid RAR5.
  if (!path_looks_rar(archive_path)) return false;
  if (file_is_rar5(archive_path)) return false;
  return true;
}

std::optional<std::vector<ArchiveMember>> read_archive_toc_unarr(
    const std::filesystem::path& archive_path) {
  const auto t0 = std::chrono::steady_clock::now();
  auto u = open_rar(archive_path);
  if (!u) return std::nullopt;

  std::vector<ArchiveMember> out;
  while (ar_parse_entry(u->ar)) {
    const char* name = ar_entry_get_name(u->ar);
    if (!name) continue;
    const std::string member = name;
    if (is_unsafe_archive_member_path(member)) continue;
    if (!member.empty() && (member.back() == '/' || member.back() == '\\'))
      continue;
    ArchiveMember m;
    m.member_path = member;
    const size_t sz = ar_entry_get_size(u->ar);
    if (sz > 0) m.uncompressed_size = static_cast<std::int64_t>(sz);
    out.push_back(std::move(m));
  }
  const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::steady_clock::now() - t0)
                      .count();
  THUMTOO_ARCHIVE_DBG("unarr TOC entries=%zu ms=%lld path=%s", out.size(),
                      static_cast<long long>(ms), archive_path.string().c_str());
  return out;
}

std::unordered_map<std::string, std::vector<std::uint8_t>>
extract_archive_members_unarr(const std::filesystem::path& archive_path,
                              const std::vector<std::string>& member_paths) {
  ScopedNsAccumulator timer(global_build_stats().archive_extract_ns);
  std::unordered_map<std::string, std::vector<std::uint8_t>> out;
  if (member_paths.empty()) return out;

  std::vector<std::string> wanted;
  for (const auto& m : member_paths) {
    if (!is_unsafe_archive_member_path(m)) wanted.push_back(m);
  }
  if (wanted.empty()) return out;

  const auto t0 = std::chrono::steady_clock::now();
  THUMTOO_ARCHIVE_DBG("unarr extract start wanted=%zu path=%s", wanted.size(),
                      archive_path.string().c_str());
  auto u = open_rar(archive_path);
  if (!u) {
    THUMTOO_ARCHIVE_DBG("unarr extract open FAILED path=%s",
                        archive_path.string().c_str());
    return out;
  }

  // Solid: walk in archive order; discard-uncompress non-wanted members so the
  // solid dictionary stays consistent (no out-of-order parse_entry_at).
  while (ar_parse_entry(u->ar) && out.size() < wanted.size()) {
    const char* path = ar_entry_get_name(u->ar);
    if (!path) continue;

    const std::string* key = nullptr;
    for (const auto& m : wanted) {
      if (out.count(m)) continue;
      if (member_paths_equal(path, m)) {
        key = &m;
        break;
      }
    }
    if (!key) {
      const size_t sz = ar_entry_get_size(u->ar);
      if (sz == 0) continue;
      if (sz > kArchiveMaxMemberUncompressedBytes) break;
      std::vector<std::uint8_t> discard(sz);
      (void)ar_entry_uncompress(u->ar, discard.data(), sz);
      continue;
    }

    const auto ts = std::chrono::steady_clock::now();
    auto buf = uncompress_current(u->ar);
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - ts)
                        .count();
    if (buf) {
      global_build_stats().archive_bytes.fetch_add(
          buf->size(), std::memory_order_relaxed);
      THUMTOO_ARCHIVE_DBG("unarr extract member ms=%lld bytes=%zu name=%s",
                          static_cast<long long>(ms), buf->size(), key->c_str());
      out.emplace(*key, std::move(*buf));
    } else {
      THUMTOO_ARCHIVE_DBG("unarr extract member FAIL ms=%lld name=%s",
                          static_cast<long long>(ms), key->c_str());
    }
  }
  const auto total_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::steady_clock::now() - t0)
                            .count();
  THUMTOO_ARCHIVE_DBG("unarr extract done got=%zu wanted=%zu total_ms=%lld path=%s",
                      out.size(), wanted.size(), static_cast<long long>(total_ms),
                      archive_path.string().c_str());
  return out;
}

std::size_t visit_archive_members_unarr(
    const std::filesystem::path& archive_path,
    const std::vector<std::string>& member_paths,
    const ArchiveMemberVisitor& visitor) {
  ScopedNsAccumulator timer(global_build_stats().archive_extract_ns);
  if (!visitor || member_paths.empty()) return 0;

  std::vector<std::string> wanted;
  for (const auto& m : member_paths) {
    if (!is_unsafe_archive_member_path(m)) wanted.push_back(m);
  }
  if (wanted.empty()) return 0;

  const auto t0 = std::chrono::steady_clock::now();
  THUMTOO_ARCHIVE_DBG("unarr visit start wanted=%zu path=%s", wanted.size(),
                      archive_path.string().c_str());
  auto u = open_rar(archive_path);
  if (!u) {
    THUMTOO_ARCHIVE_DBG("unarr visit open FAILED path=%s",
                        archive_path.string().c_str());
    return 0;
  }

  std::size_t delivered = 0;
  std::size_t solid_discards = 0;
  // Track delivered by index into wanted (path-normalized match).
  std::vector<char> done(wanted.size(), 0);
  while (ar_parse_entry(u->ar) && delivered < wanted.size()) {
    const char* path = ar_entry_get_name(u->ar);
    if (!path) continue;

    std::size_t match = wanted.size();
    for (std::size_t i = 0; i < wanted.size(); ++i) {
      if (done[i]) continue;
      if (member_paths_equal(path, wanted[i])) {
        match = i;
        break;
      }
    }
    if (match >= wanted.size()) {
      const size_t sz = ar_entry_get_size(u->ar);
      if (sz == 0) continue;
      if (sz > kArchiveMaxMemberUncompressedBytes) {
        THUMTOO_ARCHIVE_DBG("unarr visit stop oversized non-interest sz=%zu path=%s",
                            sz, path);
        break;
      }
      const auto ts = std::chrono::steady_clock::now();
      std::vector<std::uint8_t> discard(sz);
      (void)ar_entry_uncompress(u->ar, discard.data(), sz);
      ++solid_discards;
      const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::steady_clock::now() - ts)
                          .count();
      if (ms >= 50) {
        THUMTOO_ARCHIVE_DBG(
            "unarr solid-discard ms=%lld sz=%zu path=%s",
            static_cast<long long>(ms), sz, path);
      }
      continue;
    }

    const auto ts = std::chrono::steady_clock::now();
    auto buf = uncompress_current(u->ar);
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - ts)
                        .count();
    if (buf) {
      global_build_stats().archive_bytes.fetch_add(
          buf->size(), std::memory_order_relaxed);
      done[match] = 1;
      ++delivered;
      THUMTOO_ARCHIVE_DBG(
          "unarr visit [%zu/%zu] ms=%lld bytes=%zu member=%s", delivered,
          wanted.size(), static_cast<long long>(ms), buf->size(),
          wanted[match].c_str());
      visitor(wanted[match], std::move(*buf));
    } else {
      THUMTOO_ARCHIVE_DBG("unarr visit FAIL ms=%lld member=%s",
                          static_cast<long long>(ms), wanted[match].c_str());
    }
  }
  const auto total_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::steady_clock::now() - t0)
                            .count();
  THUMTOO_ARCHIVE_DBG(
      "unarr visit done delivered=%zu wanted=%zu solid_discards=%zu total_ms=%lld path=%s",
      delivered, wanted.size(), solid_discards, static_cast<long long>(total_ms),
      archive_path.string().c_str());
  return delivered;
}

}  // namespace thumtoo
