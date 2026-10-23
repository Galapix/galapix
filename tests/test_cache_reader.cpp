// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/// CacheReader + Store::open_readonly: cache-only reads next to a live writer.

#include "thumtoo/cache_reader.hpp"
#include "thumtoo/client.hpp"
#include "thumtoo/constants.hpp"
#include "thumtoo/image.hpp"
#include "thumtoo/store.hpp"
#include "thumtoo/uri.hpp"

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
      fs::temp_directory_path() / "thumtoo-cache-reader-XXXXXX";
  std::string tmpl = base.string();
  std::vector<char> buf(tmpl.begin(), tmpl.end());
  buf.push_back('\0');
  if (!mkdtemp(buf.data())) {
    std::perror("mkdtemp");
    std::exit(1);
  }
  return fs::path(buf.data());
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

bool request_tile_ok(thumtoo::Client& client, const std::string& uri, int s,
                     int x, int y) {
  bool ok = false;
  client.request_tile(uri, s, x, y,
                      [&](std::string, int, int, int,
                          std::optional<thumtoo::TileBlob> t) {
                        ok = t.has_value() && !t->bytes.empty();
                      });
  client.drain();
  return ok;
}

template <typename Fn>
bool throws(Fn&& fn) {
  try {
    fn();
  } catch (const std::exception&) {
    return true;
  }
  return false;
}

void test_parse_content_id() {
  const std::string hex(64, 'a');
  auto pure = thumtoo::parse_content_id("sha256:" + hex);
  expect(pure && pure->sha256.size() == 32 && !pure->page_1based,
         "parse_content_id pure");
  auto page = thumtoo::parse_content_id("sha256:" + hex + ":page:7");
  expect(page && page->page_1based == 7 && page->page_region_key == "7",
         "parse_content_id page");
  auto pimg = thumtoo::parse_content_id("sha256:" + hex + ":pdfimage:2");
  expect(pimg && pimg->pdfimage_1based == 2, "parse_content_id pdfimage");
  expect(!thumtoo::parse_content_id("sha256:" + hex + ":page:0"),
         "parse_content_id rejects page 0");
  expect(!thumtoo::parse_content_id("sha256:" + hex + ":bogus"),
         "parse_content_id rejects unknown suffix");
  expect(!thumtoo::parse_content_id("sha256:" + std::string(63, 'a')),
         "parse_content_id rejects short hex");
  expect(!thumtoo::parse_content_id("blob:sha256:" + hex),
         "parse_content_id rejects blob ref");
}

void test_readonly_refusals(const fs::path& root) {
  expect(throws([&] { (void)thumtoo::Store::open_readonly(root / "missing"); }),
         "open_readonly throws on missing cache");
  expect(!fs::exists(root / "missing"), "open_readonly creates nothing");

  const fs::path newer = root / "newer";
  {
    auto store = thumtoo::Store::open(newer);
    store.meta_set(thumtoo::kSchemaMetaVersionKey,
                   std::to_string(thumtoo::kStoreIndexSchemaVersion + 1));
  }
  expect(throws([&] { (void)thumtoo::Store::open_readonly(newer); }),
         "open_readonly throws on newer index schema");
  {
    auto store = thumtoo::Store::open_readonly(root / "cache");
    (void)store;
  }
  expect(fs::exists(newer / "index.sqlite"),
         "open_readonly leaves a refused cache in place");
}

}  // namespace

int main() {
  const fs::path root = make_tmpdir();
  const fs::path cache = root / "cache";
  const fs::path img = root / "wide.bmp";
  write_tiny_bmp(img, 600, 300);  // 3x2 cells at scale 0, 2x1 at scale 1

  test_parse_content_id();

  thumtoo::image_library_init();
  auto client = thumtoo::Client::open(cache, {}, 0, cache);
  const std::string uri = thumtoo::file_uri_from_path(img);
  expect(request_tile_ok(*client, uri, 0, 1, 0), "writer tile 0/1/0");
  expect(request_tile_ok(*client, uri, 1, 0, 0), "writer tile 1/0/0");

  // Reader opened while the writer Client is still live.
  auto ro = thumtoo::Store::open_readonly(cache);
  expect(ro.index_schema_version() == thumtoo::kStoreIndexSchemaVersion,
         "open_readonly schema");
  const thumtoo::CacheReader reader(ro);

  auto meta = reader.meta(uri);
  expect(meta && meta->size && meta->size->width == 600 &&
             meta->size->height == 300,
         "reader meta size");
  expect(meta && meta->format == "image", "reader meta format");
  expect(meta && meta->status == thumtoo::ContentStatus::Ready,
         "reader meta Ready with tiles");

  auto t = reader.tile(uri, 0, 1, 0);
  auto ct = client->get_tile(uri, 0, 1, 0);
  expect(t && ct && t->bytes == ct->bytes, "reader tile == Client::get_tile");
  expect(t && t->codec == "jpeg" && t->width == 256 && t->height == 256,
         "reader tile row meta");
  expect(!reader.tile(uri, 0, 0, 0), "reader miss for unstored cell");
  expect(!reader.tile("file:///nope.png", 0, 0, 0), "reader miss for unknown uri");

  auto tgt = meta ? reader.tile_target(meta->content_id) : std::nullopt;
  auto bt = tgt ? reader.tile(*tgt, 1, 0, 0) : std::nullopt;
  expect(bt && !bt->bytes.empty(), "reader tile by content_id target");

  auto cov = reader.tile_coverage(uri);
  expect(cov && cov->min_scale == 0 && cov->max_scale == 1 &&
             cov->size.width == 600,
         "reader coverage from stored scales");
  expect(reader.list_tiles(uri).size() == 2, "reader list_tiles");

  // Writes on the read-only Store fail instead of touching the cache.
  expect(throws([&] { ro.meta_set("x", "y"); }), "open_readonly rejects writes");

  // WAL: a tile the writer adds later is visible to the open reader.
  expect(request_tile_ok(*client, uri, 0, 2, 1), "writer tile 0/2/1");
  expect(reader.tile(uri, 0, 2, 1).has_value(), "reader sees later write");
  expect(reader.list_tiles(uri).size() == 3, "reader list_tiles after write");

  // Locations: prefix match, paging, unrelated prefixes.
  const fs::path img2 = root / "wide2.bmp";
  write_tiny_bmp(img2, 32, 32);
  const std::string uri2 = thumtoo::file_uri_from_path(img2);
  expect(request_tile_ok(*client, uri2, 0, 0, 0), "writer tile for second image");
  const std::string prefix = thumtoo::file_uri_from_path(root) + "/";
  auto all = reader.list_locations(prefix);
  expect(all.size() == 2 && all[0] == uri && all[1] == uri2,
         "list_locations prefix, sorted");
  auto page1 = reader.list_locations(prefix, {}, 1);
  auto page2 = reader.list_locations(prefix, page1.empty() ? "" : page1[0], 1);
  expect(page1.size() == 1 && page2.size() == 1 && page2[0] == uri2,
         "list_locations paging");
  expect(reader.list_locations(prefix + "zzz").empty(),
         "list_locations empty for unrelated prefix");
  expect(!reader.list_locations({}).empty(), "list_locations all");

  test_readonly_refusals(root);

  client.reset();
  std::error_code ec;
  fs::remove_all(root, ec);
  if (g_failures) {
    std::cerr << g_failures << " failure(s)\n";
    return 1;
  }
  std::cout << "cache_reader OK\n";
  return 0;
}
