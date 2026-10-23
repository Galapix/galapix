// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/// Read-only HTTP server for a thumtoo cache (docs/REMOTE.md): locations,
/// their metadata and stored tiles. Never generates anything; a miss is 404.

#include "thumtoo/cache_reader.hpp"
#include "thumtoo/remote.hpp"
#include "thumtoo/store.hpp"
#include "thumtoo/version.hpp"

#include <httplib.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <string_view>
#include <vector>

#include <sys/stat.h>
#include <unistd.h>

namespace fs = std::filesystem;
using nlohmann::json;

namespace {

constexpr int kDefaultPort = 8080;
constexpr int kDefaultListLimit = 1000;
constexpr int kMaxListLimit = 5000;

struct Options {
  fs::path cache;
  std::string host = "127.0.0.1";
  int port = kDefaultPort;
  std::vector<std::string> allow_prefixes;
  std::optional<fs::path> token_file;
  bool no_auth = false;
  int threads = 8;
  bool verbose = false;
};

fs::path default_cache_root() {
  const char* xdg = std::getenv("XDG_CACHE_HOME");
  if (xdg && *xdg) return fs::path(xdg) / "thumtoo";
  const char* home = std::getenv("HOME");
  if (home && *home) return fs::path(home) / ".cache" / "thumtoo";
  return fs::path(".cache") / "thumtoo";
}

void usage(const char* argv0) {
  std::cerr
      << "thumtoo " << thumtoo::version_string() << "\n"
      << "Usage: " << argv0 << " [OPTIONS]\n\n"
      << "  Serve a thumtoo cache read-only over HTTP (locations, metadata,\n"
      << "  stored tiles). Nothing is generated; missing tiles are 404.\n\n"
      << "  --cache DIR          Cache root (default $XDG_CACHE_HOME/thumtoo)\n"
      << "  --listen HOST:PORT   Bind address (default 127.0.0.1:8080)\n"
      << "  --allow-prefix URI   Only expose locations below URI (repeatable)\n"
      << "  --token-file FILE    Read the access token from FILE, creating it\n"
      << "                       with a random token if missing\n"
      << "  --no-auth            No token, even on a non-loopback address\n"
      << "  --threads N          Request threads (default 8)\n"
      << "  -v, --verbose        Log every request\n"
      << "  -V, --version        Print version\n\n"
      << "  A loopback bind needs no token. Any other bind requires one (random\n"
      << "  per start unless --token-file); the printed URL carries it as\n"
      << "  ?token=. Plain HTTP only: tunnel (ssh -L, Tailscale) to encrypt.\n";
}

bool is_loopback(std::string_view host) {
  return host == "localhost" || host == "::1" || host.starts_with("127.");
}

std::string random_token() {
  std::random_device rd;
  std::string out;
  static constexpr char kHex[] = "0123456789abcdef";
  for (int i = 0; i < 16; ++i) {
    const unsigned v = rd() & 0xff;
    out.push_back(kHex[v >> 4]);
    out.push_back(kHex[v & 0xf]);
  }
  return out;
}

std::string load_or_create_token(const fs::path& file) {
  if (std::ifstream in(file); in) {
    std::string token;
    std::getline(in, token);
    while (!token.empty() && std::isspace(static_cast<unsigned char>(token.back()))) {
      token.pop_back();
    }
    if (token.empty()) throw std::runtime_error(file.string() + ": empty token");
    return token;
  }
  const std::string token = random_token();
  const mode_t old_mask = ::umask(077);
  std::ofstream out(file);
  ::umask(old_mask);
  if (!out) throw std::runtime_error(file.string() + ": cannot create");
  out << token << '\n';
  return token;
}

bool constant_time_equal(std::string_view a, std::string_view b) {
  if (a.size() != b.size()) return false;
  unsigned char diff = 0;
  for (std::size_t i = 0; i < a.size(); ++i) {
    diff |= static_cast<unsigned char>(a[i] ^ b[i]);
  }
  return diff == 0;
}

std::optional<std::string> cookie_value(const httplib::Request& req,
                                        std::string_view name) {
  const std::string header = req.get_header_value("Cookie");
  std::string_view rest = header;
  while (!rest.empty()) {
    const auto semi = rest.find(';');
    std::string_view kv = rest.substr(0, semi);
    while (kv.starts_with(' ')) kv.remove_prefix(1);
    if (kv.size() > name.size() && kv.substr(0, name.size()) == name &&
        kv[name.size()] == '=') {
      return std::string(kv.substr(name.size() + 1));
    }
    if (semi == std::string_view::npos) break;
    rest.remove_prefix(semi + 1);
  }
  return std::nullopt;
}

class Server {
 public:
  Server(Options opts, std::string token)
      : opts_(std::move(opts)), token_(std::move(token)) {}

  /// Per-thread read-only Store: SQLite connections are not shared.
  thumtoo::CacheReader& reader() {
    thread_local std::unique_ptr<thumtoo::Store> store;
    thread_local std::unique_ptr<thumtoo::CacheReader> reader;
    if (!reader) {
      store = std::make_unique<thumtoo::Store>(
          thumtoo::Store::open_readonly(opts_.cache));
      reader = std::make_unique<thumtoo::CacheReader>(*store);
      store_of_thread() = store.get();
    }
    return *reader;
  }

  thumtoo::Store& store() {
    (void)reader();
    return *store_of_thread();
  }

  bool allowed(std::string_view uri) const {
    if (opts_.allow_prefixes.empty()) return true;
    return std::any_of(opts_.allow_prefixes.begin(), opts_.allow_prefixes.end(),
                       [&](const std::string& p) { return uri.starts_with(p); });
  }

  /// Pre-routing auth. Returns false (and fills res) when rejected.
  bool authorize(const httplib::Request& req, httplib::Response& res) const {
    if (token_.empty()) return true;
    if (req.has_param("token") &&
        constant_time_equal(req.get_param_value("token"), token_)) {
      // Browsers keep the cookie, so a pasted ?token= URL works once.
      res.set_header("Set-Cookie", std::string(thumtoo::kRemoteTokenCookie) +
                                       "=" + token_ +
                                       "; Path=/; HttpOnly; SameSite=Strict");
      return true;
    }
    const std::string auth = req.get_header_value("Authorization");
    constexpr std::string_view kBearer = "Bearer ";
    if (std::string_view(auth).starts_with(kBearer) &&
        constant_time_equal(std::string_view(auth).substr(kBearer.size()),
                            token_)) {
      return true;
    }
    if (auto c = cookie_value(req, thumtoo::kRemoteTokenCookie);
        c && constant_time_equal(*c, token_)) {
      return true;
    }
    res.status = 401;
    res.set_content("missing or wrong token\n", "text/plain");
    return false;
  }

  std::optional<thumtoo::RemoteInfo> info_for(std::string_view uri) {
    auto loc = store().find_locator(uri);
    if (!loc || !loc->blob_id) return std::nullopt;
    thumtoo::RemoteInfo info;
    info.uri = loc->uri;
    info.outer_path = loc->outer_path;
    info.member_path = loc->member_path;
    info.file_size = loc->size;
    info.mtime_ns = loc->mtime_ns;
    auto& r = reader();
    info.meta = r.meta(uri);
    info.coverage = r.tile_coverage(uri);
    info.lqip = r.lqip(uri);
    return info;
  }

  void handle_hello(const httplib::Request&, httplib::Response& res) const {
    json j;
    j["service"] = "thumtoo-serve";
    j["api"] = thumtoo::kRemoteApiVersion;
    j["version"] = std::string(thumtoo::version_string());
    j["tile_size"] = 256;
    res.set_content(j.dump(), "application/json");
  }

  void handle_locations(const httplib::Request& req, httplib::Response& res) {
    int limit = kDefaultListLimit;
    if (req.has_param("limit")) {
      limit = std::clamp(std::atoi(req.get_param_value("limit").c_str()), 1,
                         kMaxListLimit);
    }
    const std::string after = req.get_param_value("after");
    std::string prefix = req.get_param_value("prefix");
    // One exported prefix: scan only below it instead of filtering it out.
    if (opts_.allow_prefixes.size() == 1 &&
        opts_.allow_prefixes.front().starts_with(prefix)) {
      prefix = opts_.allow_prefixes.front();
    }
    const std::vector<std::string> scanned =
        req.has_param("like")
            ? store().list_locator_uris_like(req.get_param_value("like"), after,
                                             limit)
            : store().list_locator_uris(prefix, after, limit);
    thumtoo::RemoteLocations page;
    for (const auto& uri : scanned) {
      if (!allowed(uri)) continue;
      if (auto info = info_for(uri)) page.items.push_back(std::move(*info));
    }
    if (static_cast<int>(scanned.size()) == limit) page.next = scanned.back();
    res.set_content(thumtoo::remote_locations_to_json(page), "application/json");
  }

  void handle_route(const httplib::Request& req, httplib::Response& res) {
    auto route = thumtoo::parse_remote_route(req.path);
    if (!route) {
      res.status = 404;
      res.set_content("unknown route\n", "text/plain");
      return;
    }
    if (!route->by_content_id && !allowed(route->location)) {
      res.status = 404;
      res.set_content("not found\n", "text/plain");
      return;
    }
    switch (route->kind) {
      case thumtoo::RemoteRoute::Kind::Info:
        return handle_info(*route, res);
      case thumtoo::RemoteRoute::Kind::Tiles:
        return handle_tiles(*route, res);
      case thumtoo::RemoteRoute::Kind::Tile:
        return handle_tile(*route, req, res);
    }
  }

  void handle_info(const thumtoo::RemoteRoute& route, httplib::Response& res) {
    if (route.by_content_id) {
      res.status = 404;
      res.set_content("//info needs a location\n", "text/plain");
      return;
    }
    auto info = info_for(route.location);
    if (!info) {
      res.status = 404;
      res.set_content("location not in cache\n", "text/plain");
      return;
    }
    res.set_content(thumtoo::remote_info_to_json(*info), "application/json");
  }

  void handle_tiles(const thumtoo::RemoteRoute& route, httplib::Response& res) {
    std::vector<thumtoo::Store::TileRow> rows;
    if (route.by_content_id) {
      if (auto tgt = reader().tile_target(route.location)) {
        rows = store().list_tiles_for_region(tgt->media_id, tgt->region_id);
      }
    } else {
      rows = reader().list_tiles(route.location);
    }
    json tiles = json::array();
    for (const auto& t : rows) {
      tiles.push_back({{"scale", t.scale},
                       {"x", t.x},
                       {"y", t.y},
                       {"width", t.width},
                       {"height", t.height},
                       {"codec", thumtoo::tile_codec_name(t.codec_id)}});
    }
    res.set_content(json{{"tiles", std::move(tiles)}}.dump(), "application/json");
  }

  void handle_tile(const thumtoo::RemoteRoute& route,
                   const httplib::Request& req, httplib::Response& res) {
    auto& r = reader();
    std::string content_id;
    std::optional<thumtoo::CacheReader::TileTarget> tgt;
    if (route.by_content_id) {
      content_id = route.location;
    } else if (auto m = r.meta(route.location)) {
      content_id = m->content_id;
    }
    if (!content_id.empty()) tgt = r.tile_target(content_id);
    auto tile = tgt ? r.tile(*tgt, route.scale, route.x, route.y) : std::nullopt;
    if (!tile) {
      res.status = 404;
      res.set_content("tile not in cache\n", "text/plain");
      return;
    }
    // Tiles are keyed by content: same id + cell = same bytes.
    const std::string etag = "\"" + content_id + "/" + std::to_string(route.scale) +
                             "/" + std::to_string(route.x) + "/" +
                             std::to_string(route.y) + "\"";
    res.set_header("ETag", etag);
    res.set_header("Cache-Control", route.by_content_id
                                        ? "public, max-age=31536000, immutable"
                                        : "no-cache");
    if (req.get_header_value("If-None-Match") == etag) {
      res.status = 304;
      return;
    }
    res.set_header("X-Tile-Width", std::to_string(tile->width));
    res.set_header("X-Tile-Height", std::to_string(tile->height));
    res.set_content(std::string(tile->bytes.begin(), tile->bytes.end()),
                    thumtoo::tile_codec_mime(tile->codec));
  }

  const Options& options() const { return opts_; }

 private:
  static thumtoo::Store*& store_of_thread() {
    thread_local thumtoo::Store* p = nullptr;
    return p;
  }

  Options opts_;
  std::string token_;
};

bool parse_listen(std::string_view s, Options* opts) {
  const auto colon = s.rfind(':');
  if (colon == std::string_view::npos) return false;
  std::string host(s.substr(0, colon));
  if (host.size() >= 2 && host.front() == '[' && host.back() == ']') {
    host = host.substr(1, host.size() - 2);
  }
  const int port = std::atoi(std::string(s.substr(colon + 1)).c_str());
  if (host.empty() || port <= 0 || port > 65535) return false;
  opts->host = host;
  opts->port = port;
  return true;
}

std::string display_host(const Options& opts) {
  if (opts.host == "0.0.0.0" || opts.host == "::") {
    char name[256] = {};
    if (::gethostname(name, sizeof(name) - 1) == 0 && name[0]) return name;
  }
  if (opts.host.find(':') != std::string::npos) return "[" + opts.host + "]";
  return opts.host;
}

}  // namespace

int main(int argc, char** argv) {
  Options opts;
  opts.cache = default_cache_root();
  for (int i = 1; i < argc; ++i) {
    const std::string_view a = argv[i];
    auto next = [&]() -> std::string {
      if (i + 1 >= argc) {
        std::cerr << "thumtoo-serve: " << a << " needs an argument\n";
        std::exit(2);
      }
      return argv[++i];
    };
    if (a == "-h" || a == "--help") {
      usage(argv[0]);
      return 0;
    } else if (a == "-V" || a == "--version") {
      thumtoo::print_version(std::cout);
      return 0;
    } else if (a == "--cache") {
      opts.cache = next();
    } else if (a == "--listen") {
      if (!parse_listen(next(), &opts)) {
        std::cerr << "thumtoo-serve: --listen wants HOST:PORT\n";
        return 2;
      }
    } else if (a == "--allow-prefix") {
      opts.allow_prefixes.push_back(next());
    } else if (a == "--token-file") {
      opts.token_file = fs::path(next());
    } else if (a == "--no-auth") {
      opts.no_auth = true;
    } else if (a == "--threads") {
      opts.threads = std::max(1, std::atoi(next().c_str()));
    } else if (a == "-v" || a == "--verbose") {
      opts.verbose = true;
    } else {
      std::cerr << "thumtoo-serve: unknown argument " << a << "\n";
      usage(argv[0]);
      return 2;
    }
  }

  std::string token;
  try {
    // Fail early (missing cache, schema mismatch) instead of per request.
    (void)thumtoo::Store::open_readonly(opts.cache);
    if (opts.token_file && !opts.no_auth) {
      token = load_or_create_token(*opts.token_file);
    } else if (!opts.no_auth && !is_loopback(opts.host)) {
      token = random_token();
    }
  } catch (const std::exception& ex) {
    std::cerr << "thumtoo-serve: " << ex.what() << "\n";
    return 1;
  }

  Server server(opts, token);
  httplib::Server http;
  const int threads = opts.threads;
  http.new_task_queue = [threads] { return new httplib::ThreadPool(threads); };
  http.set_default_headers({{"Access-Control-Allow-Origin", "*"}});
  http.set_pre_routing_handler(
      [&server](const httplib::Request& req, httplib::Response& res) {
        return server.authorize(req, res)
                   ? httplib::Server::HandlerResponse::Unhandled
                   : httplib::Server::HandlerResponse::Handled;
      });
  http.set_exception_handler([](const httplib::Request&, httplib::Response& res,
                                std::exception_ptr ep) {
    std::string what = "internal error";
    try {
      std::rethrow_exception(ep);
    } catch (const std::exception& ex) {
      what = ex.what();
    } catch (...) {
    }
    res.status = 500;
    res.set_content(what + "\n", "text/plain");
  });
  if (opts.verbose) {
    http.set_logger([](const httplib::Request& req, const httplib::Response& res) {
      std::fprintf(stderr, "%d %s %s\n", res.status, req.method.c_str(),
                   req.path.c_str());
    });
  }
  http.Get("/", [&server](const httplib::Request& req, httplib::Response& res) {
    server.handle_hello(req, res);
  });
  http.Get("/locations",
           [&server](const httplib::Request& req, httplib::Response& res) {
             server.handle_locations(req, res);
           });
  http.Get(R"(/.+)", [&server](const httplib::Request& req, httplib::Response& res) {
    server.handle_route(req, res);
  });

  if (!http.bind_to_port(opts.host, opts.port)) {
    std::cerr << "thumtoo-serve: cannot listen on " << opts.host << ":"
              << opts.port << "\n";
    return 1;
  }
  std::string url = "http://" + display_host(opts) + ":" + std::to_string(opts.port) + "/";
  if (!token.empty()) url += "?token=" + token;
  std::cout << "thumtoo-serve: " << opts.cache.string() << " on " << url << "\n";
  if (!opts.allow_prefixes.empty()) {
    for (const auto& p : opts.allow_prefixes) {
      std::cout << "thumtoo-serve: exporting " << p << "\n";
    }
  }
  if (token.empty() && !is_loopback(opts.host)) {
    std::cout << "thumtoo-serve: WARNING no token on a network address\n";
  }
  std::cout << std::flush;
  return http.listen_after_bind() ? 0 : 1;
}
