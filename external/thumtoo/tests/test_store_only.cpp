// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/// THUMTOO_STORE_ONLY: no legacy Database; Store-first probe for file images.

#include "thumtoo/client.hpp"
#include "thumtoo/constants.hpp"
#include "thumtoo/image.hpp"
#include "thumtoo/layout.hpp"
#include "thumtoo/store.hpp"
#include "thumtoo/uri.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <unistd.h>

namespace fs = std::filesystem;

namespace {

int g_failures = 0;

void expect(bool cond, const char* msg) {
  if (!cond) {
    std::cerr << "FAIL: " << msg << "\n";
    ++g_failures;
  }
}

fs::path make_tmpdir() {
  const fs::path base =
      fs::temp_directory_path() / "thumtoo-store-only-XXXXXX";
  std::string tmpl = base.string();
  std::vector<char> buf(tmpl.begin(), tmpl.end());
  buf.push_back('\0');
  if (!mkdtemp(buf.data())) {
    std::perror("mkdtemp");
    std::exit(1);
  }
  return fs::path(buf.data());
}

bool file_nonempty(const fs::path& p) {
  std::error_code ec;
  return fs::is_regular_file(p, ec) && fs::file_size(p, ec) > 0;
}

void write_tiny_bmp(const fs::path& path, int w, int h) {
  const int row_raw = w * 3;
  const int row_pad = (4 - (row_raw % 4)) % 4;
  const int row_stride = row_raw + row_pad;
  const int pixel_size = row_stride * h;
  const int file_size = 14 + 40 + pixel_size;
  std::vector<unsigned char> buf(static_cast<size_t>(file_size), 0);
  auto put16 = [&](int off, int v) {
    buf[static_cast<size_t>(off)] = static_cast<unsigned char>(v & 0xff);
    buf[static_cast<size_t>(off + 1)] = static_cast<unsigned char>((v >> 8) & 0xff);
  };
  auto put32 = [&](int off, int v) {
    put16(off, v & 0xffff);
    put16(off + 2, (v >> 16) & 0xffff);
  };
  buf[0] = 'B';
  buf[1] = 'M';
  put32(2, file_size);
  put32(10, 54);
  put32(14, 40);
  put32(18, w);
  put32(22, h);
  put16(26, 1);
  put16(28, 24);
  put32(34, pixel_size);
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      const int i = 54 + y * row_stride + x * 3;
      buf[static_cast<size_t>(i) + 0] = 10;
      buf[static_cast<size_t>(i) + 1] = 20;
      buf[static_cast<size_t>(i) + 2] = 30;
    }
  }
  std::ofstream out(path, std::ios::binary);
  out.write(reinterpret_cast<const char*>(buf.data()),
            static_cast<std::streamsize>(buf.size()));
}

}  // namespace

int main() {
  unsetenv("THUMTOO_STORE_ONLY");  // default on
  unsetenv("THUMTOO_STORE_ROOT");  // default top-level Store

  const fs::path root = make_tmpdir();
  const fs::path cache = root / "cache";
  const fs::path img = root / "tiny.bmp";
  write_tiny_bmp(img, 64, 48);

  auto client = thumtoo::Client::open(cache, {}, 0, cache);
  expect(client != nullptr, "Client::open STORE_ONLY");

  expect(file_nonempty(cache / "index.sqlite"), "Store index on disk");
  expect(file_nonempty(cache / "bulk.sqlite"), "Store bulk on disk");
  expect(!fs::exists(cache / "legacy"), "no legacy/ directory");
  expect(client->store().index_schema_version() ==
             thumtoo::kStoreIndexSchemaVersion,
         "Store schema");


  thumtoo::image_library_init();
  const auto uri = thumtoo::file_uri_from_path(img);

  bool size_ok = false;
  client->request_size(uri, [&](std::string, thumtoo::SizeReply r) {
    size_ok = r.size.has_value() && r.size->width == 64 && r.size->height == 48;
  });
  client->drain();
  expect(size_ok, "Store-only request_size for file image");

  auto sz = client->get_size(uri);
  expect(sz && sz->width == 64 && sz->height == 48, "get_size from Store");

  bool px_ok = false;
  client->request_pixels(uri, 64, [&](std::string, int, std::optional<thumtoo::PixelLevel> p) {
    px_ok = p.has_value() && !p->bytes.empty();
  });
  client->drain();
  expect(px_ok, "Store-only request_pixels session encode");

  bool tile_ok = false;
  client->request_tile(uri, 0, 0, 0,
                       [&](std::string, int, int, int,
                           std::optional<thumtoo::TileBlob> t) {
                         tile_ok = t.has_value() && !t->bytes.empty();
                       });
  client->drain();
  expect(tile_ok, "Store-only request_tile scale0 cell");
  expect(client->has_tile(uri, 0, 0, 0), "has_tile after Store durable write");
  auto t0 = client->get_tile(uri, 0, 0, 0);
  expect(t0.has_value() && !t0->bytes.empty(), "get_tile from Store");

  // Tags on Store only (no legacy).
  expect(client->add_tag(uri, "store-only-tag"), "add_tag Store-only");
  auto tags = client->get_tags(uri);
  expect(std::find(tags.begin(), tags.end(), "store-only-tag") != tags.end(),
         "get_tags sees Store tag");
  expect(client->remove_tag(uri, "store-only-tag"), "remove_tag Store-only");
  tags = client->get_tags(uri);
  expect(std::find(tags.begin(), tags.end(), "store-only-tag") == tags.end(),
         "tag removed");

  // Still no legacy files after probe/pixels/tiles/tags.
  expect(!fs::exists(cache / "legacy"), "no legacy/ after work");

  if (g_failures) {
    std::cerr << g_failures << " failure(s)\n";
    return 1;
  }
  std::cout << "OK test_store_only\n";
  return 0;
}
