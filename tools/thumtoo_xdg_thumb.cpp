// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "thumtoo/xdg_thumbnail.hpp"
#include "thumtoo/version.hpp"

#include <iostream>
#include <string>

static void usage(const char* argv0) {
  std::cerr
      << "Usage: " << argv0 << " [OPTION]... PATH\n"
      << "XDG thumbnail cache lookup / Thumbnailer1 request (not thumtoo tiles).\n"
      << "\n"
      << "  -h, --help       show this help\n"
      << "      --version    version and features\n"
      << "      --flavor F   normal|large|x-large|xx-large (default: large)\n"
      << "      --mime M     MIME type for Queue (default: application/octet-stream)\n"
      << "      --request    Queue via D-Bus if cache miss (needs THUMTOO_HAVE_DBUS)\n"
      << "      --force      delete cache then request\n"
      << "      --timeout MS wait for daemon (default: 15000)\n";
}

int main(int argc, char** argv) {
  thumtoo::XdgThumbnailFlavor flavor = thumtoo::XdgThumbnailFlavor::Large;
  std::string mime = "application/octet-stream";
  bool do_request = false;
  bool force = false;
  int timeout_ms = 15000;
  std::string path;

  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    if (a == "-h" || a == "--help") {
      usage(argv[0]);
      return 0;
    }
    if (a == "--version") {
      thumtoo::print_version(std::cout);
      return 0;
    }
    if (a == "--request") {
      do_request = true;
      continue;
    }
    if (a == "--force") {
      force = true;
      continue;
    }
    if (a == "--flavor" && i + 1 < argc) {
      const std::string f = argv[++i];
      if (f == "normal") {
        flavor = thumtoo::XdgThumbnailFlavor::Normal;
      } else if (f == "large") {
        flavor = thumtoo::XdgThumbnailFlavor::Large;
      } else if (f == "x-large") {
        flavor = thumtoo::XdgThumbnailFlavor::XLarge;
      } else if (f == "xx-large") {
        flavor = thumtoo::XdgThumbnailFlavor::XXLarge;
      } else {
        std::cerr << "unknown flavor: " << f << '\n';
        return 2;
      }
      continue;
    }
    if (a == "--mime" && i + 1 < argc) {
      mime = argv[++i];
      continue;
    }
    if (a == "--timeout" && i + 1 < argc) {
      timeout_ms = std::atoi(argv[++i]);
      continue;
    }
    if (!a.empty() && a[0] == '-') {
      std::cerr << "unknown option: " << a << '\n';
      return 2;
    }
    path = a;
  }
  if (path.empty()) {
    usage(argv[0]);
    return 2;
  }

  const std::filesystem::path file(path);
  std::cout << "uri:   " << thumtoo::xdg_file_uri(file) << '\n';
  std::cout << "cache: " << thumtoo::xdg_thumbnail_cache_path(file, flavor).string()
            << '\n';
  if (auto hit = thumtoo::xdg_thumbnail_lookup(file, flavor)) {
    std::cout << "hit:   " << hit->string() << " (fresh)\n";
    if (!do_request) {
      return 0;
    }
  } else {
    std::cout << "hit:   (none)\n";
  }

  if (!do_request) {
    return 1;  // miss
  }

  thumtoo::XdgThumbnailer thumb;
  std::cout << "dbus:  " << (thumtoo::xdg_thumbnail_dbus_built() ? "built" : "not built")
            << ", service " << (thumb.service_available() ? "up" : "down") << '\n';
  auto r = thumb.request_wait(file, mime, flavor, timeout_ms, force);
  if (r.path) {
    std::cout << "png:   " << r.path->string()
              << (r.from_cache ? " (cache)\n" : " (generated)\n");
    return 0;
  }
  std::cerr << "error: " << (r.error.empty() ? "unknown" : r.error) << '\n';
  return 1;
}
