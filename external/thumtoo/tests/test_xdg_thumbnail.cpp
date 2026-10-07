// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "thumtoo/xdg_thumbnail.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

static int fails = 0;
#define CHECK(cond)                                                            \
  do {                                                                         \
    if (!(cond)) {                                                             \
      std::cerr << "FAIL " << __FILE__ << ":" << __LINE__ << " " #cond << '\n'; \
      ++fails;                                                                 \
    }                                                                          \
  } while (0)

int main() {
  // file:// URI encoding: space → %20
  {
    const auto uri = thumtoo::xdg_file_uri("/tmp/hello world.png");
    CHECK(uri.find("file://") == 0);
    CHECK(uri.find("%20") != std::string::npos);
    CHECK(uri.find(' ') == std::string::npos);
  }

  // Cache path is under XDG_CACHE_HOME/thumbnails/<flavor>/<md5>.png
  {
    setenv("XDG_CACHE_HOME", "/tmp/thumtoo-xdg-test-cache", 1);
    const auto p = thumtoo::xdg_thumbnail_cache_path("/tmp/foo.jpg",
                                                     thumtoo::XdgThumbnailFlavor::Large);
    CHECK(p.string().find("/tmp/thumtoo-xdg-test-cache/thumbnails/large/") == 0);
    CHECK(p.extension() == ".png");
    // Same URI → same path
    const auto p2 = thumtoo::xdg_thumbnail_cache_path("/tmp/foo.jpg",
                                                      thumtoo::XdgThumbnailFlavor::Large);
    CHECK(p == p2);
  }

  // Lookup miss on absent file
  {
    auto hit = thumtoo::xdg_thumbnail_lookup("/tmp/thumtoo-no-such-file-xyz.jpg");
    CHECK(!hit.has_value());
  }

  // Flavor names match XDG subdirs
  {
    CHECK(std::string(thumtoo::xdg_thumbnail_flavor_name(
              thumtoo::XdgThumbnailFlavor::Normal)) == "normal");
    CHECK(std::string(thumtoo::xdg_thumbnail_flavor_name(
              thumtoo::XdgThumbnailFlavor::Large)) == "large");
    CHECK(std::string(thumtoo::xdg_thumbnail_flavor_name(
              thumtoo::XdgThumbnailFlavor::XLarge)) == "x-large");
    CHECK(std::string(thumtoo::xdg_thumbnail_flavor_name(
              thumtoo::XdgThumbnailFlavor::XXLarge)) == "xx-large");
  }

  // Different paths → different digests; same path different flavors → different dirs
  {
    setenv("XDG_CACHE_HOME", "/tmp/thumtoo-xdg-test-cache2", 1);
    const auto a = thumtoo::xdg_thumbnail_cache_path("/tmp/a.jpg");
    const auto b = thumtoo::xdg_thumbnail_cache_path("/tmp/b.jpg");
    CHECK(a.filename() != b.filename());
    const auto n = thumtoo::xdg_thumbnail_cache_path(
        "/tmp/a.jpg", thumtoo::XdgThumbnailFlavor::Normal);
    const auto l = thumtoo::xdg_thumbnail_cache_path(
        "/tmp/a.jpg", thumtoo::XdgThumbnailFlavor::Large);
    CHECK(n.filename() == l.filename());
    CHECK(n.parent_path().filename() == "normal");
    CHECK(l.parent_path().filename() == "large");
  }

  // remove_cache on missing files is zero / non-negative
  {
    const int n = thumtoo::xdg_thumbnail_remove_cache("/tmp/thumtoo-no-thumb-xyz.jpg");
    CHECK(n >= 0);
  }

  // D-Bus feature flag is consistent
  {
    CHECK(thumtoo::xdg_thumbnail_dbus_built() ==
          (THUMTOO_HAVE_DBUS != 0));
  }

  if (fails) {
    std::cerr << fails << " failure(s)\n";
    return 1;
  }
  std::cout << "ok\n";
  return 0;
}
