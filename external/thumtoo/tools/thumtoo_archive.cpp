// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
//
// CLI for archive TOC + extract using thumtoo's archive backends
// (libarchive + optional libunarr for solid RAR/CBR).
//
// Usage:
//   thumtoo-archive list ARCHIVE
//   thumtoo-archive info ARCHIVE
//   thumtoo-archive cat ARCHIVE MEMBER
//   thumtoo-archive extract ARCHIVE [-C DIR] [MEMBER...]
//   thumtoo-archive extract-all ARCHIVE [-C DIR]
//
// Exit codes: 0 ok, 1 usage/error, 2 archive open/TOC failed, 3 extract failed.

#include "thumtoo/archive.hpp"
#include "thumtoo/version.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;

namespace {

void usage(const char* argv0) {
  std::cerr
      << "Usage:\n"
      << "  " << argv0 << " list ARCHIVE\n"
      << "  " << argv0 << " info ARCHIVE\n"
      << "  " << argv0 << " cat ARCHIVE MEMBER\n"
      << "  " << argv0 << " extract ARCHIVE [-C DIR] [MEMBER...]\n"
      << "  " << argv0 << " extract-all ARCHIVE [-C DIR]\n"
      << "\n"
      << "Uses thumtoo archive backends: libunarr for .rar/.cbr when available,\n"
      << "otherwise libarchive. Same paths as biltoo //archive: URIs.\n";
}

std::string backend_label(const fs::path& archive) {
  if (thumtoo::unarr_backend_available() &&
      thumtoo::archive_prefers_unarr(archive)) {
    return "unarr (preferred for this path)";
  }
  if (thumtoo::unarr_backend_available()) {
    return "libarchive (unarr linked, not preferred for this path)";
  }
  return "libarchive (unarr not linked)";
}

int cmd_info(const fs::path& archive) {
  std::cout << "path:     " << archive.string() << "\n";
  std::cout << "exists:   " << (fs::is_regular_file(archive) ? "yes" : "no")
            << "\n";
  std::cout << "backend:  " << backend_label(archive) << "\n";
  std::cout << "unarr:    "
            << (thumtoo::unarr_backend_available() ? "available" : "not linked")
            << "\n";
  std::cout << "prefers_unarr: "
            << (thumtoo::archive_prefers_unarr(archive) ? "yes" : "no") << "\n";

  auto toc = thumtoo::read_archive_toc(archive);
  if (!toc) {
    std::cerr << "error: failed to read TOC\n";
    return 2;
  }
  std::size_t images = 0;
  std::uint64_t total = 0;
  for (const auto& m : *toc) {
    if (thumtoo::is_likely_image_member_path(m.member_path)) ++images;
    if (m.uncompressed_size && *m.uncompressed_size > 0)
      total += static_cast<std::uint64_t>(*m.uncompressed_size);
  }
  std::cout << "members:  " << toc->size() << "\n";
  std::cout << "images:   " << images << "\n";
  if (total > 0)
    std::cout << "sum_size: " << total << " bytes (declared)\n";
  return 0;
}

int cmd_list(const fs::path& archive) {
  auto toc = thumtoo::read_archive_toc(archive);
  if (!toc) {
    std::cerr << "error: failed to read TOC (unsupported, solid without unarr, "
                 "or I/O error)\n";
    std::cerr << "backend: " << backend_label(archive) << "\n";
    return 2;
  }
  for (const auto& m : *toc) {
    const char mark =
        thumtoo::is_likely_image_member_path(m.member_path) ? '*' : ' ';
    if (m.uncompressed_size && *m.uncompressed_size >= 0) {
      std::printf("%c %10lld  %s\n", mark,
                  static_cast<long long>(*m.uncompressed_size),
                  m.member_path.c_str());
    } else {
      std::printf("%c %10s  %s\n", mark, "-", m.member_path.c_str());
    }
  }
  std::cerr << "# " << toc->size() << " members, backend="
            << backend_label(archive) << " (* = image)\n";
  return 0;
}

bool write_file(const fs::path& dest, const std::vector<std::uint8_t>& data) {
  if (dest.has_parent_path()) {
    std::error_code ec;
    fs::create_directories(dest.parent_path(), ec);
  }
  std::ofstream out(dest, std::ios::binary);
  if (!out) return false;
  out.write(reinterpret_cast<const char*>(data.data()),
            static_cast<std::streamsize>(data.size()));
  return static_cast<bool>(out);
}

int cmd_cat(const fs::path& archive, std::string_view member) {
  auto bytes = thumtoo::extract_archive_member(archive, member);
  if (!bytes) {
    std::cerr << "error: extract failed for member: " << member << "\n";
    std::cerr << "backend: " << backend_label(archive) << "\n";
    return 3;
  }
  if (std::fwrite(bytes->data(), 1, bytes->size(), stdout) != bytes->size()) {
    std::cerr << "error: short write to stdout\n";
    return 1;
  }
  return 0;
}

int cmd_extract(const fs::path& archive, const fs::path& out_dir,
                const std::vector<std::string>& members, bool all) {
  std::vector<std::string> want = members;
  if (all) {
    auto toc = thumtoo::read_archive_toc(archive);
    if (!toc) {
      std::cerr << "error: failed to read TOC\n";
      return 2;
    }
    want.clear();
    want.reserve(toc->size());
    for (const auto& m : *toc) want.push_back(m.member_path);
  }
  if (want.empty()) {
    std::cerr << "error: no members to extract\n";
    return 1;
  }

  auto map = thumtoo::extract_archive_members(archive, want);
  int ok = 0;
  int fail = 0;
  for (const auto& name : want) {
    auto it = map.find(name);
    if (it == map.end()) {
      std::cerr << "FAIL  " << name << "\n";
      ++fail;
      continue;
    }
    fs::path rel = name;
    if (rel.is_absolute()) rel = rel.filename();
    fs::path dest = out_dir / rel;
    if (!write_file(dest, it->second)) {
      std::cerr << "FAIL  write " << dest.string() << "\n";
      ++fail;
      continue;
    }
    std::cout << "OK    " << dest.string() << " (" << it->second.size()
              << " bytes)\n";
    ++ok;
  }
  std::cerr << "# extracted " << ok << " ok, " << fail
            << " failed, backend=" << backend_label(archive) << "\n";
  return fail ? 3 : 0;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc >= 2) {
    const std::string a0 = argv[1];
    if (a0 == "--version" || a0 == "-V") {
      thumtoo::print_version(std::cout);
      return 0;
    }
    if (a0 == "--help" || a0 == "-h") {
      // fall through to usage via argc < 3
    }
  }
  if (argc < 3) {
    usage(argv[0]);
    return 1;
  }
  const std::string cmd = argv[1];
  const fs::path archive = argv[2];

  if (cmd == "list" || cmd == "ls" || cmd == "tvf") {
    return cmd_list(archive);
  }
  if (cmd == "info") {
    return cmd_info(archive);
  }
  if (cmd == "cat") {
    if (argc < 4) {
      usage(argv[0]);
      return 1;
    }
    return cmd_cat(archive, argv[3]);
  }
  if (cmd == "extract" || cmd == "x") {
    fs::path out_dir = ".";
    std::vector<std::string> members;
    for (int i = 3; i < argc; ++i) {
      std::string a = argv[i];
      if ((a == "-C" || a == "--directory") && i + 1 < argc) {
        out_dir = argv[++i];
        continue;
      }
      members.push_back(a);
    }
    return cmd_extract(archive, out_dir, members, /*all=*/false);
  }
  if (cmd == "extract-all" || cmd == "xa") {
    fs::path out_dir = ".";
    for (int i = 3; i < argc; ++i) {
      std::string a = argv[i];
      if ((a == "-C" || a == "--directory") && i + 1 < argc) {
        out_dir = argv[++i];
        continue;
      }
      std::cerr << "error: unexpected argument: " << a << "\n";
      return 1;
    }
    return cmd_extract(archive, out_dir, {}, /*all=*/true);
  }

  usage(argv[0]);
  return 1;
}
