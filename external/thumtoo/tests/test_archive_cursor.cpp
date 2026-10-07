// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "thumtoo/archive.hpp"
#include "thumtoo/constants.hpp"

#include <cstdio>
#include <string>
#include <vector>

static int g_fail = 0;

static void expect(bool cond, const char* msg) {
  if (!cond) {
    std::fprintf(stderr, "FAIL: %s\n", msg);
    ++g_fail;
  }
}

int main() {
  const std::vector<std::string> toc = {
      "a/01.jpg", "a/02.jpg", "a/03.jpg", "a/04.jpg", "a/05.jpg",
      "a/06.jpg", "a/07.jpg", "a/08.jpg", "a/09.jpg", "a/10.jpg",
  };

  // Empty interest continues from cursor.
  {
    auto w = thumtoo::plan_archive_batch_window(toc, {}, 3, 4);
    expect(w.size() == 4, "empty interest size");
    expect(w[0] == "a/04.jpg", "empty interest start at next_index");
    expect(w.back() == "a/07.jpg", "empty interest end");
  }

  // Interest span within window → contiguous TOC span.
  {
    auto w = thumtoo::plan_archive_batch_window(
        toc, {"a/03.jpg", "a/05.jpg"}, 0, 32);
    expect(w.size() == 3, "small interest fills span");
    expect(w[0] == "a/03.jpg" && w[1] == "a/04.jpg" && w[2] == "a/05.jpg",
           "small interest contiguous");
  }

  // Interest wider than max_window → capped.
  {
    auto w = thumtoo::plan_archive_batch_window(
        toc, {"a/01.jpg", "a/10.jpg"}, 0, 4);
    expect(w.size() == 4, "wide interest capped");
    expect(w.front() == "a/01.jpg", "wide interest from lo");
  }

  // Cursor anchors the window when span exceeds max.
  {
    auto w = thumtoo::plan_archive_batch_window(
        toc, {"a/01.jpg", "a/10.jpg"}, 7, 3);
    expect(w.size() == 3, "anchor window size");
    // Anchor near index 7 → members around 07-09 region of full span.
    bool has7 = false;
    for (const auto& m : w)
      if (m == "a/07.jpg" || m == "a/08.jpg" || m == "a/06.jpg") has7 = true;
    expect(has7, "anchor near next_index");
  }

  // Unknown interest ignored.
  {
    auto w = thumtoo::plan_archive_batch_window(
        toc, {"missing.jpg"}, 0, 8);
    expect(w.empty() || w.size() <= 8, "unknown interest handled");
    // With only unknown interest, idxs empty → continue from 0.
    expect(w.size() == 8, "unknown falls back to cursor continuum");
    expect(w[0] == "a/01.jpg", "unknown fallback start");
  }

  // TOC index helper.
  {
    auto i = thumtoo::archive_member_toc_index(toc, "a/05.jpg");
    expect(i && *i == 4, "toc index 5th");
    expect(!thumtoo::archive_member_toc_index(toc, "nope"), "toc miss");
  }


  // Archive access class heuristic.
  {
    using thumtoo::ArchiveAccess;
    using thumtoo::archive_access_class;
    expect(archive_access_class("/x/a.zip") == ArchiveAccess::Random, "zip random");
    expect(archive_access_class("/x/a.CBZ") == ArchiveAccess::Random, "cbz random");
    expect(archive_access_class("/x/a.tar") == ArchiveAccess::Sequential, "tar seq");
    expect(archive_access_class("/x/a.tar.gz") == ArchiveAccess::Sequential,
           "tar.gz seq");
    expect(archive_access_class("/x/a.tgz") == ArchiveAccess::Sequential, "tgz seq");
    expect(archive_access_class("/x/a.rar") == ArchiveAccess::Sequential, "rar seq");
    expect(archive_access_class("/x/a.cbr") == ArchiveAccess::Sequential, "cbr seq");
    expect(archive_access_class("/x/a.7z") == ArchiveAccess::Sequential, "7z seq");
    expect(archive_access_class("/x/a.cb7") == ArchiveAccess::Sequential, "cb7 seq");
    // Unknown → Sequential (safe).
    expect(archive_access_class("/x/a.unknown") == ArchiveAccess::Sequential,
           "unknown seq");
  }

  expect(thumtoo::kBatchWindowMembers == 32, "window constant");
  expect(thumtoo::kBatchMaxEdge == 1024, "batch edge constant");

  if (g_fail) {
    std::fprintf(stderr, "%d failure(s)\n", g_fail);
    return 1;
  }
  std::printf("test_archive_cursor: ok\n");
  return 0;
}
