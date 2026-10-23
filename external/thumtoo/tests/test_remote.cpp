// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/// Remote Client against a spawned thumtoo-serve (docs/REMOTE.md).

#include "thumtoo/client.hpp"
#include "thumtoo/image.hpp"
#include "thumtoo/remote.hpp"
#include "thumtoo/uri.hpp"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#ifndef THUMTOO_SERVE_BIN
#error "THUMTOO_SERVE_BIN must point at the thumtoo-serve binary"
#endif

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
  std::string tmpl =
      (fs::temp_directory_path() / "thumtoo-remote-XXXXXX").string();
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

int free_port() {
  const int fd = ::socket(AF_INET, SOCK_STREAM, 0);
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  addr.sin_port = 0;
  socklen_t len = sizeof(addr);
  ::bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
  ::getsockname(fd, reinterpret_cast<sockaddr*>(&addr), &len);
  const int port = ntohs(addr.sin_port);
  ::close(fd);
  return port;
}

bool port_open(int port) {
  const int fd = ::socket(AF_INET, SOCK_STREAM, 0);
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  addr.sin_port = htons(static_cast<std::uint16_t>(port));
  const bool ok =
      ::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0;
  ::close(fd);
  return ok;
}

/// thumtoo-serve child process; killed on destruction.
class ServeProcess {
 public:
  ServeProcess(const fs::path& cache, std::vector<std::string> extra)
      : port_(free_port()) {
    std::vector<std::string> args = {THUMTOO_SERVE_BIN, "--cache", cache.string(),
                                     "--listen",
                                     "127.0.0.1:" + std::to_string(port_)};
    args.insert(args.end(), extra.begin(), extra.end());
    pid_ = ::fork();
    if (pid_ == 0) {
      if (!std::freopen("/dev/null", "w", stdout)) std::_Exit(126);
      std::vector<char*> argv;
      for (auto& a : args) argv.push_back(a.data());
      argv.push_back(nullptr);
      ::execv(argv[0], argv.data());
      std::_Exit(127);
    }
    for (int i = 0; i < 100 && !port_open(port_); ++i) {
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
  }
  ~ServeProcess() {
    if (pid_ > 0) {
      ::kill(pid_, SIGTERM);
      ::waitpid(pid_, nullptr, 0);
    }
  }
  std::string url() const {
    return "http://127.0.0.1:" + std::to_string(port_) + "/";
  }

 private:
  int port_;
  pid_t pid_ = -1;
};

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

std::vector<thumtoo::TileResult> fetch_cells(
    thumtoo::Client& client, const std::string& uri,
    std::vector<thumtoo::Client::TileCoord> coords) {
  std::vector<thumtoo::TileResult> out(coords.size());
  client.request_tile_cells(uri, std::move(coords),
                            [&](std::size_t i, thumtoo::TileResult r) {
                              out[i] = std::move(r);
                            });
  client.drain();
  return out;
}

void test_protocol() {
  thumtoo::RemoteRoute tile;
  tile.kind = thumtoo::RemoteRoute::Kind::Tile;
  tile.location = "file:///a b/c#d%e.pdf//page:3";
  tile.scale = -2;
  tile.x = 4;
  tile.y = 5;
  const std::string path = thumtoo::format_remote_route(tile);
  expect(path == "/file:///a%20b/c%23d%25e.pdf//page:3//tile:-2,4,5",
         "format_remote_route escapes, keeps // pipes");
  auto back = thumtoo::parse_remote_route("/file:///a b/c#d%e.pdf//page:3//tile:-2,4,5");
  expect(back && back->kind == thumtoo::RemoteRoute::Kind::Tile &&
             back->location == tile.location && back->scale == -2 &&
             back->x == 4 && back->y == 5,
         "parse_remote_route tile");
  auto info = thumtoo::parse_remote_route("/file:///x.jpg//info");
  expect(info && info->kind == thumtoo::RemoteRoute::Kind::Info &&
             info->location == "file:///x.jpg",
         "parse_remote_route info");
  auto blob = thumtoo::parse_remote_route("/blob/sha256:ab//tile:0,0,0");
  expect(blob && blob->by_content_id && blob->location == "sha256:ab",
         "parse_remote_route blob");
  expect(!thumtoo::parse_remote_route("/file:///x.jpg//tile:0,0"),
         "parse_remote_route rejects short coords");
  expect(!thumtoo::parse_remote_route("/file:///x.jpg"),
         "parse_remote_route rejects bare location");

  for (std::size_t n = 0; n < 8; ++n) {
    std::vector<std::uint8_t> data;
    for (std::size_t i = 0; i < n; ++i) data.push_back(static_cast<std::uint8_t>(i * 77 + 3));
    auto rt = thumtoo::base64_decode(thumtoo::base64_encode(data));
    expect(rt && *rt == data, "base64 roundtrip");
  }
}

}  // namespace

int main() {
  test_protocol();
  if (!thumtoo::remote_available()) {
    std::cout << "remote OK (no remote support built, protocol only)\n";
    return g_failures ? 1 : 0;
  }

  const fs::path root = make_tmpdir();
  const fs::path cache = root / "cache";
  const fs::path img = root / "wide.bmp";
  const fs::path odd = root / "odd name #1.bmp";
  write_tiny_bmp(img, 600, 300);
  write_tiny_bmp(odd, 40, 30);

  thumtoo::image_library_init();
  const std::string uri = thumtoo::file_uri_from_path(img);
  const std::string odd_uri = thumtoo::file_uri_from_path(odd);
  std::optional<thumtoo::TileBlob> local_tile;
  {
    auto local = thumtoo::Client::open(cache, {}, 0, cache);
    expect(request_tile_ok(*local, uri, 0, 1, 0), "local tile 0/1/0");
    expect(request_tile_ok(*local, uri, 1, 0, 0), "local tile 1/0/0");
    expect(request_tile_ok(*local, odd_uri, 0, 0, 0), "local tile odd name");
    local_tile = local->get_tile(uri, 0, 1, 0);
  }
  if (local_tile) local_tile = thumtoo::decode_tile_blob_to_rgb888(*local_tile);

  const fs::path token_file = root / "token";
  ServeProcess serve(cache, {"--token-file", token_file.string()});
  std::string token;
  std::ifstream(token_file) >> token;
  expect(!token.empty(), "server wrote token file");

  bool rejected = false;
  try {
    (void)thumtoo::Client::open_remote(serve.url());
  } catch (const std::exception&) {
    rejected = true;
  }
  expect(rejected, "open_remote without token is rejected");

  {
    auto remote = thumtoo::Client::open_remote(serve.url() + "?token=" + token);
    expect(remote->is_remote(), "is_remote");
    expect(!remote->get_size(uri), "get_size before listing is a memo miss");

    auto rows = remote->list_locators(1000);
    expect(rows.size() == 2, "list_locators sees both images");
    auto sz = remote->get_size(uri);
    expect(sz && sz->width == 600 && sz->height == 300,
           "get_size from listing memo");
    auto cov = remote->get_tile_coverage(uri);
    expect(cov && cov->min_scale == 0 && cov->max_scale == 1,
           "get_tile_coverage from listing memo");

    auto cells = fetch_cells(*remote, uri, {{0, 1, 0}, {1, 0, 0}, {0, 0, 0}});
    expect(cells[0].status == thumtoo::TileStatus::Ok && cells[0].tile &&
               local_tile && cells[0].tile->bytes == local_tile->bytes,
           "remote cell matches local decode");
    expect(cells[1].status == thumtoo::TileStatus::Ok, "remote coarse cell");
    expect(cells[2].status == thumtoo::TileStatus::Unavailable,
           "unstored cell is Unavailable");

    auto prefix = remote->list_locators_by_uri_prefix(
        thumtoo::file_uri_from_path(root) + "/", 1000);
    expect(prefix.size() == 2, "list_locators_by_uri_prefix");
    auto by_path = remote->list_locators_by_outer_path_prefix(root.string(), 1000);
    expect(by_path.size() == 2, "list_locators_by_outer_path_prefix");
    auto like = remote->list_locators_like("%odd%", 1000);
    expect(like.size() == 1 && like[0].uri == odd_uri, "list_locators_like");
    auto paged = remote->list_locators(1);
    expect(paged.size() == 1, "list_locators honours limit");
  }

  {
    // Fresh memo: request_size goes over the wire (escaped ' ' and '#').
    auto remote = thumtoo::Client::open_remote(serve.url() + "?token=" + token);
    std::optional<thumtoo::Size> got;
    remote->request_size(odd_uri, [&](std::string, thumtoo::SizeReply r) {
      got = r.size;
    });
    bool missing_answered = false;
    remote->request_size("file:///no/such.jpg",
                         [&](std::string, thumtoo::SizeReply r) {
                           missing_answered = !r.size;
                         });
    remote->drain();
    expect(got && got->width == 40 && got->height == 30,
           "request_size over the wire, escaped location");
    expect(missing_answered, "request_size miss answers empty");
    expect(remote->get_size(odd_uri).has_value(), "request_size fills memo");
  }

  {
    ServeProcess limited(cache, {"--no-auth", "--allow-prefix", uri});
    auto remote = thumtoo::Client::open_remote(limited.url());
    auto rows = remote->list_locators(1000);
    expect(rows.size() == 1 && rows[0].uri == uri, "--allow-prefix filters listing");
    auto cells = fetch_cells(*remote, odd_uri, {{0, 0, 0}});
    expect(cells[0].status == thumtoo::TileStatus::Unavailable,
           "--allow-prefix hides other tiles");
  }

  std::error_code ec;
  fs::remove_all(root, ec);
  if (g_failures) {
    std::cerr << g_failures << " failure(s)\n";
    return 1;
  }
  std::cout << "remote OK\n";
  return 0;
}
