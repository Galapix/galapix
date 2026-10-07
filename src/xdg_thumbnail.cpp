// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "thumtoo/xdg_thumbnail.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <mutex>
#include <unordered_map>
#include <vector>

#include <sys/stat.h>

#if defined(THUMTOO_HAVE_DBUS) && THUMTOO_HAVE_DBUS
#include <dbus/dbus.h>
#endif

namespace thumtoo {
namespace {

// --- MD5 (RFC 1321, compact) -------------------------------------------------

struct Md5Ctx {
  std::uint32_t state[4]{};
  std::uint64_t bitlen = 0;
  std::uint8_t buffer[64]{};
  std::size_t buflen = 0;
};

constexpr std::uint32_t md5_rol(std::uint32_t x, int n) {
  return (x << n) | (x >> (32 - n));
}

void md5_transform(std::uint32_t state[4], const std::uint8_t block[64]) {
  static constexpr std::uint32_t K[64] = {
      0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf, 0x4787c62a,
      0xa8304613, 0xfd469501, 0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
      0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821, 0xf61e2562, 0xc040b340,
      0x265e5a51, 0xe9b6c7aa, 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
      0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8,
      0x676f02d9, 0x8d2a4c8a, 0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
      0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70, 0x289b7ec6, 0xeaa127fa,
      0xd4ef3085, 0x04881d05, 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
      0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92,
      0xffeff47d, 0x85845dd1, 0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
      0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391};
  static constexpr int S[64] = {
      7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
      5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20,
      4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
      6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21};

  std::uint32_t M[16];
  for (int i = 0; i < 16; ++i) {
    M[i] = static_cast<std::uint32_t>(block[i * 4]) |
           (static_cast<std::uint32_t>(block[i * 4 + 1]) << 8) |
           (static_cast<std::uint32_t>(block[i * 4 + 2]) << 16) |
           (static_cast<std::uint32_t>(block[i * 4 + 3]) << 24);
  }
  std::uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
  for (int i = 0; i < 64; ++i) {
    std::uint32_t f = 0, g = 0;
    if (i < 16) {
      f = (b & c) | ((~b) & d);
      g = static_cast<std::uint32_t>(i);
    } else if (i < 32) {
      f = (d & b) | ((~d) & c);
      g = static_cast<std::uint32_t>((5 * i + 1) % 16);
    } else if (i < 48) {
      f = b ^ c ^ d;
      g = static_cast<std::uint32_t>((3 * i + 5) % 16);
    } else {
      f = c ^ (b | (~d));
      g = static_cast<std::uint32_t>((7 * i) % 16);
    }
    const std::uint32_t t = d;
    d = c;
    c = b;
    b = b + md5_rol(a + f + K[i] + M[g], S[i]);
    a = t;
  }
  state[0] += a;
  state[1] += b;
  state[2] += c;
  state[3] += d;
}

void md5_init(Md5Ctx& ctx) {
  ctx.state[0] = 0x67452301;
  ctx.state[1] = 0xefcdab89;
  ctx.state[2] = 0x98badcfe;
  ctx.state[3] = 0x10325476;
  ctx.bitlen = 0;
  ctx.buflen = 0;
}

void md5_update(Md5Ctx& ctx, const std::uint8_t* data, std::size_t len) {
  ctx.bitlen += static_cast<std::uint64_t>(len) * 8;
  while (len > 0) {
    const std::size_t n = std::min(len, 64 - ctx.buflen);
    std::memcpy(ctx.buffer + ctx.buflen, data, n);
    ctx.buflen += n;
    data += n;
    len -= n;
    if (ctx.buflen == 64) {
      md5_transform(ctx.state, ctx.buffer);
      ctx.buflen = 0;
    }
  }
}

std::array<std::uint8_t, 16> md5_final(Md5Ctx& ctx) {
  ctx.buffer[ctx.buflen++] = 0x80;
  if (ctx.buflen > 56) {
    while (ctx.buflen < 64) {
      ctx.buffer[ctx.buflen++] = 0;
    }
    md5_transform(ctx.state, ctx.buffer);
    ctx.buflen = 0;
  }
  while (ctx.buflen < 56) {
    ctx.buffer[ctx.buflen++] = 0;
  }
  for (int i = 0; i < 8; ++i) {
    ctx.buffer[56 + i] = static_cast<std::uint8_t>((ctx.bitlen >> (8 * i)) & 0xff);
  }
  md5_transform(ctx.state, ctx.buffer);
  std::array<std::uint8_t, 16> out{};
  for (int i = 0; i < 4; ++i) {
    out[i * 4 + 0] = static_cast<std::uint8_t>(ctx.state[i] & 0xff);
    out[i * 4 + 1] = static_cast<std::uint8_t>((ctx.state[i] >> 8) & 0xff);
    out[i * 4 + 2] = static_cast<std::uint8_t>((ctx.state[i] >> 16) & 0xff);
    out[i * 4 + 3] = static_cast<std::uint8_t>((ctx.state[i] >> 24) & 0xff);
  }
  return out;
}

std::string md5_hex(std::string_view data) {
  Md5Ctx ctx;
  md5_init(ctx);
  md5_update(ctx, reinterpret_cast<const std::uint8_t*>(data.data()), data.size());
  const auto dig = md5_final(ctx);
  static constexpr char kHex[] = "0123456789abcdef";
  std::string out(32, '0');
  for (int i = 0; i < 16; ++i) {
    out[static_cast<std::size_t>(i * 2)] = kHex[dig[static_cast<std::size_t>(i)] >> 4];
    out[static_cast<std::size_t>(i * 2 + 1)] = kHex[dig[static_cast<std::size_t>(i)] & 0xf];
  }
  return out;
}

// --- URI / cache paths -------------------------------------------------------

bool is_unreserved(unsigned char c) {
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
         c == '-' || c == '_' || c == '.' || c == '~' || c == '/';
}

std::string percent_encode_path(std::string_view path) {
  static constexpr char kHex[] = "0123456789ABCDEF";
  std::string out;
  out.reserve(path.size() + 16);
  for (unsigned char c : path) {
    if (is_unreserved(c)) {
      out.push_back(static_cast<char>(c));
    } else {
      out.push_back('%');
      out.push_back(kHex[c >> 4]);
      out.push_back(kHex[c & 0xf]);
    }
  }
  return out;
}

std::filesystem::path xdg_cache_home() {
  if (const char* e = std::getenv("XDG_CACHE_HOME"); e && e[0]) {
    return std::filesystem::path(e);
  }
  if (const char* home = std::getenv("HOME"); home && home[0]) {
    return std::filesystem::path(home) / ".cache";
  }
  return std::filesystem::path(".cache");
}

std::uint32_t read_be32(const std::uint8_t* p) {
  return (std::uint32_t(p[0]) << 24) | (std::uint32_t(p[1]) << 16) |
         (std::uint32_t(p[2]) << 8) | std::uint32_t(p[3]);
}

std::optional<std::string> png_text_chunk(const std::filesystem::path& path,
                                          const char* key) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    return std::nullopt;
  }
  std::vector<char> data((std::istreambuf_iterator<char>(in)),
                         std::istreambuf_iterator<char>());
  if (data.size() > 2 * 1024 * 1024) {
    data.resize(2 * 1024 * 1024);
  }
  static const unsigned char sig[8] = {137, 80, 78, 71, 13, 10, 26, 10};
  if (data.size() < 8 ||
      std::memcmp(data.data(), sig, 8) != 0) {
    return std::nullopt;
  }
  const std::string key_s(key);
  std::size_t off = 8;
  while (off + 12 <= data.size()) {
    const auto len = read_be32(reinterpret_cast<const std::uint8_t*>(data.data() + off));
    if (off + 12 + len > data.size()) {
      break;
    }
    const char* type = data.data() + off + 4;
    const char* chunk = data.data() + off + 8;
    if (std::memcmp(type, "tEXt", 4) == 0 && len > 0) {
      const std::string_view body(chunk, len);
      const auto sep = body.find('\0');
      if (sep != std::string_view::npos && sep > 0 &&
          body.substr(0, sep) == key_s) {
        return std::string(body.substr(sep + 1));
      }
    }
    if (std::memcmp(type, "IEND", 4) == 0) {
      break;
    }
    off += 12 + len;
  }
  return std::nullopt;
}

}  // namespace

const char* xdg_thumbnail_flavor_name(XdgThumbnailFlavor flavor) noexcept {
  switch (flavor) {
    case XdgThumbnailFlavor::Normal:
      return "normal";
    case XdgThumbnailFlavor::Large:
      return "large";
    case XdgThumbnailFlavor::XLarge:
      return "x-large";
    case XdgThumbnailFlavor::XXLarge:
      return "xx-large";
  }
  return "large";
}

std::string xdg_file_uri(const std::filesystem::path& absolute_file) {
  std::error_code ec;
  auto abs = std::filesystem::absolute(absolute_file, ec);
  if (ec) {
    abs = absolute_file;
  }
  abs = abs.lexically_normal();
  std::string native = abs.string();
  // file:// + absolute path (leading / kept)
  if (native.empty() || native[0] != '/') {
    native = "/" + native;
  }
  return "file://" + percent_encode_path(native);
}

std::filesystem::path xdg_thumbnail_cache_path(const std::filesystem::path& absolute_file,
                                               XdgThumbnailFlavor flavor) {
  const std::string uri = xdg_file_uri(absolute_file);
  const std::string digest = md5_hex(uri);
  return xdg_cache_home() / "thumbnails" / xdg_thumbnail_flavor_name(flavor) /
         (digest + ".png");
}

bool xdg_thumbnail_cache_is_fresh(const std::filesystem::path& absolute_file,
                                  const std::filesystem::path& cache_png) {
  std::error_code ec;
  if (!std::filesystem::is_regular_file(cache_png, ec) || ec) {
    return false;
  }
  if (!std::filesystem::is_regular_file(absolute_file, ec) || ec) {
    return true;
  }
  // Thumb::MTime is Unix epoch seconds (Freedesktop thumbnail spec).
  struct stat src_st {};
  struct stat cache_st {};
  if (stat(absolute_file.c_str(), &src_st) != 0) {
    return true;
  }
  const std::int64_t src_mtime = static_cast<std::int64_t>(src_st.st_mtime);
  const std::int64_t src_size = static_cast<std::int64_t>(src_st.st_size);

  if (auto mt = png_text_chunk(cache_png, "Thumb::MTime")) {
    try {
      if (std::stoll(*mt) != src_mtime) {
        return false;
      }
    } catch (...) {
    }
  } else if (stat(cache_png.c_str(), &cache_st) == 0) {
    if (src_mtime > static_cast<std::int64_t>(cache_st.st_mtime)) {
      return false;
    }
  }

  if (auto sz = png_text_chunk(cache_png, "Thumb::Size")) {
    try {
      if (std::stoll(*sz) != src_size) {
        return false;
      }
    } catch (...) {
    }
  }
  return true;
}

std::optional<std::filesystem::path> xdg_thumbnail_lookup(
    const std::filesystem::path& absolute_file, XdgThumbnailFlavor flavor) {
  const auto path = xdg_thumbnail_cache_path(absolute_file, flavor);
  if (xdg_thumbnail_cache_is_fresh(absolute_file, path)) {
    return path;
  }
  return std::nullopt;
}

int xdg_thumbnail_remove_cache(const std::filesystem::path& absolute_file) {
  static constexpr XdgThumbnailFlavor kFlavors[] = {
      XdgThumbnailFlavor::Normal, XdgThumbnailFlavor::Large,
      XdgThumbnailFlavor::XLarge, XdgThumbnailFlavor::XXLarge};
  int removed = 0;
  std::error_code ec;
  for (auto f : kFlavors) {
    const auto p = xdg_thumbnail_cache_path(absolute_file, f);
    if (std::filesystem::remove(p, ec)) {
      ++removed;
    }
  }
  // fail/ store (same MD5 name)
  const std::string uri = xdg_file_uri(absolute_file);
  const auto fail =
      xdg_cache_home() / "thumbnails" / "fail" / (md5_hex(uri) + ".png");
  if (std::filesystem::remove(fail, ec)) {
    ++removed;
  }
  return removed;
}

// --- D-Bus client ------------------------------------------------------------

struct PendingRequest {
  std::vector<XdgThumbnailCallback> callbacks;
  XdgThumbnailFlavor flavor = XdgThumbnailFlavor::Large;
};

struct XdgThumbnailer::Impl {
  Executor executor;
  std::mutex mu;
  std::unordered_map<std::string, PendingRequest> by_uri;
  std::vector<std::uint32_t> handles;

#if defined(THUMTOO_HAVE_DBUS) && THUMTOO_HAVE_DBUS
  DBusConnection* conn = nullptr;
  bool service_ok = false;

  bool ensure_bus() {
    if (conn) {
      return true;
    }
    DBusError err;
    dbus_error_init(&err);
    conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
    if (dbus_error_is_set(&err) || !conn) {
      dbus_error_free(&err);
      conn = nullptr;
      return false;
    }
    dbus_connection_set_exit_on_disconnect(conn, FALSE);
    // Match Thumbnailer1 signals
    dbus_bus_add_match(
        conn,
        "type='signal',interface='org.freedesktop.thumbnails.Thumbnailer1'",
        &err);
    if (dbus_error_is_set(&err)) {
      dbus_error_free(&err);
    }
    dbus_connection_flush(conn);
    return true;
  }

  bool ensure_service() {
    if (!ensure_bus()) {
      service_ok = false;
      return false;
    }
    if (service_ok) {
      return true;
    }
    DBusError err;
    dbus_error_init(&err);
    // Activate name if needed
    dbus_bus_start_service_by_name(conn, "org.freedesktop.thumbnails.Thumbnailer1",
                                   0, nullptr, &err);
    if (dbus_error_is_set(&err)) {
      dbus_error_free(&err);
    }
    service_ok =
        dbus_bus_name_has_owner(conn, "org.freedesktop.thumbnails.Thumbnailer1",
                                nullptr) != FALSE;
    return service_ok;
  }

  void deliver(XdgThumbnailReply reply) {
    std::vector<XdgThumbnailCallback> cbs;
    {
      std::lock_guard lock(mu);
      auto it = by_uri.find(reply.uri);
      if (it == by_uri.end()) {
        return;
      }
      cbs = std::move(it->second.callbacks);
      by_uri.erase(it);
    }
    for (auto& cb : cbs) {
      if (!cb) {
        continue;
      }
      XdgThumbnailReply copy = reply;
      auto fn = cb;
      executor.post([fn = std::move(fn), copy = std::move(copy)]() mutable {
        fn(std::move(copy));
      });
    }
  }

  [[nodiscard]] XdgThumbnailFlavor pending_flavor(const std::string& uri) {
    std::lock_guard lock(mu);
    auto it = by_uri.find(uri);
    if (it != by_uri.end()) {
      return it->second.flavor;
    }
    return XdgThumbnailFlavor::Large;
  }

  void handle_signal(DBusMessage* msg) {
    const char* member = dbus_message_get_member(msg);
    if (!member) {
      return;
    }
    if (std::strcmp(member, "Ready") == 0) {
      dbus_uint32_t handle = 0;
      DBusMessageIter iter;
      dbus_message_iter_init(msg, &iter);
      if (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_UINT32) {
        dbus_message_iter_get_basic(&iter, &handle);
        dbus_message_iter_next(&iter);
      }
      if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_ARRAY) {
        return;
      }
      DBusMessageIter arr;
      dbus_message_iter_recurse(&iter, &arr);
      while (dbus_message_iter_get_arg_type(&arr) == DBUS_TYPE_STRING) {
        const char* uri = nullptr;
        dbus_message_iter_get_basic(&arr, &uri);
        if (uri) {
          XdgThumbnailReply r;
          r.uri = uri;
          // Prefer requested flavor, then xx-large → normal
          // URI → path is reverse of file:// only for local files
          std::filesystem::path file;
          if (r.uri.size() > 7 && r.uri.compare(0, 7, "file://") == 0) {
            // minimal decode for lookup of alternate flavors
            std::string path_s = r.uri.substr(7);
            // leave percent-decoding simple: replace %XX
            std::string decoded;
            for (std::size_t i = 0; i < path_s.size(); ++i) {
              if (path_s[i] == '%' && i + 2 < path_s.size()) {
                auto hex = [](char c) -> int {
                  if (c >= '0' && c <= '9') return c - '0';
                  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
                  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
                  return -1;
                };
                const int hi = hex(path_s[i + 1]);
                const int lo = hex(path_s[i + 2]);
                if (hi >= 0 && lo >= 0) {
                  decoded.push_back(static_cast<char>((hi << 4) | lo));
                  i += 2;
                  continue;
                }
              }
              decoded.push_back(path_s[i]);
            }
            file = decoded;
          }
          if (!file.empty()) {
            const XdgThumbnailFlavor want = pending_flavor(r.uri);
            static const XdgThumbnailFlavor kOrder[] = {
                XdgThumbnailFlavor::XXLarge, XdgThumbnailFlavor::XLarge,
                XdgThumbnailFlavor::Large, XdgThumbnailFlavor::Normal};
            std::error_code ec;
            auto try_flavor = [&](XdgThumbnailFlavor f) {
              const auto p = xdg_thumbnail_cache_path(file, f);
              if (std::filesystem::is_regular_file(p, ec)) {
                r.path = p;
                return true;
              }
              return false;
            };
            if (!try_flavor(want)) {
              for (auto f : kOrder) {
                if (f != want && try_flavor(f)) {
                  break;
                }
              }
            }
          }
          if (!r.path) {
            r.error = "Ready signal but cache file missing";
          }
          deliver(std::move(r));
        }
        dbus_message_iter_next(&arr);
      }
      (void)handle;
    } else if (std::strcmp(member, "Error") == 0) {
      DBusMessageIter iter;
      dbus_message_iter_init(msg, &iter);
      // u handle, as uris, i code, s message
      if (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_UINT32) {
        dbus_message_iter_next(&iter);
      }
      if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_ARRAY) {
        return;
      }
      DBusMessageIter arr;
      dbus_message_iter_recurse(&iter, &arr);
      std::vector<std::string> uris;
      while (dbus_message_iter_get_arg_type(&arr) == DBUS_TYPE_STRING) {
        const char* uri = nullptr;
        dbus_message_iter_get_basic(&arr, &uri);
        if (uri) {
          uris.emplace_back(uri);
        }
        dbus_message_iter_next(&arr);
      }
      dbus_message_iter_next(&iter);
      int code = 0;
      if (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_INT32) {
        dbus_message_iter_get_basic(&iter, &code);
        dbus_message_iter_next(&iter);
      }
      const char* message = "";
      if (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_STRING) {
        dbus_message_iter_get_basic(&iter, &message);
      }
      for (const auto& uri : uris) {
        XdgThumbnailReply r;
        r.uri = uri;
        r.error = std::string("[") + std::to_string(code) + "] " +
                  (message ? message : "");
        deliver(std::move(r));
      }
    } else if (std::strcmp(member, "Finished") == 0) {
      dbus_uint32_t handle = 0;
      DBusMessageIter iter;
      if (dbus_message_iter_init(msg, &iter) &&
          dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_UINT32) {
        dbus_message_iter_get_basic(&iter, &handle);
        std::lock_guard lock(mu);
        handles.erase(std::remove(handles.begin(), handles.end(), handle),
                      handles.end());
      }
    }
  }

  void dispatch(int timeout_ms) {
    if (!conn) {
      return;
    }
    dbus_connection_read_write_dispatch(conn, timeout_ms);
    while (true) {
      DBusMessage* msg = dbus_connection_pop_message(conn);
      if (!msg) {
        break;
      }
      if (dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_SIGNAL) {
        handle_signal(msg);
      }
      dbus_message_unref(msg);
    }
  }

  std::optional<std::uint32_t> queue(const std::vector<std::string>& uris,
                                      const std::vector<std::string>& mimes,
                                      const char* flavor) {
    if (!ensure_service() || uris.empty()) {
      return std::nullopt;
    }
    DBusMessage* msg = dbus_message_new_method_call(
        "org.freedesktop.thumbnails.Thumbnailer1",
        "/org/freedesktop/thumbnails/Thumbnailer1",
        "org.freedesktop.thumbnails.Thumbnailer1", "Queue");
    if (!msg) {
      return std::nullopt;
    }
    DBusMessageIter args;
    dbus_message_iter_init_append(msg, &args);
    DBusMessageIter arr;
    dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, "s", &arr);
    for (const auto& u : uris) {
      const char* p = u.c_str();
      dbus_message_iter_append_basic(&arr, DBUS_TYPE_STRING, &p);
    }
    dbus_message_iter_close_container(&args, &arr);
    dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, "s", &arr);
    for (std::size_t i = 0; i < uris.size(); ++i) {
      const char* p =
          (i < mimes.size() ? mimes[i].c_str() : "application/octet-stream");
      dbus_message_iter_append_basic(&arr, DBUS_TYPE_STRING, &p);
    }
    dbus_message_iter_close_container(&args, &arr);
    const char* flav = flavor;
    const char* sched = "default";
    dbus_uint32_t dequeue_handle = 0;
    dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &flav);
    dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &sched);
    dbus_message_iter_append_basic(&args, DBUS_TYPE_UINT32, &dequeue_handle);

    DBusError err;
    dbus_error_init(&err);
    DBusMessage* reply =
        dbus_connection_send_with_reply_and_block(conn, msg, 5000, &err);
    dbus_message_unref(msg);
    if (!reply || dbus_error_is_set(&err)) {
      if (dbus_error_is_set(&err)) {
        dbus_error_free(&err);
      }
      if (reply) {
        dbus_message_unref(reply);
      }
      service_ok = false;
      return std::nullopt;
    }
    dbus_uint32_t handle = 0;
    if (!dbus_message_get_args(reply, &err, DBUS_TYPE_UINT32, &handle,
                               DBUS_TYPE_INVALID)) {
      dbus_message_unref(reply);
      if (dbus_error_is_set(&err)) {
        dbus_error_free(&err);
      }
      return std::nullopt;
    }
    dbus_message_unref(reply);
    std::lock_guard lock(mu);
    handles.push_back(handle);
    return handle;
  }

  void dequeue_all() {
    if (!conn) {
      return;
    }
    std::vector<std::uint32_t> hs;
    {
      std::lock_guard lock(mu);
      hs = handles;
      handles.clear();
    }
    for (auto h : hs) {
      DBusMessage* msg = dbus_message_new_method_call(
          "org.freedesktop.thumbnails.Thumbnailer1",
          "/org/freedesktop/thumbnails/Thumbnailer1",
          "org.freedesktop.thumbnails.Thumbnailer1", "Dequeue");
      if (!msg) {
        continue;
      }
      dbus_message_append_args(msg, DBUS_TYPE_UINT32, &h, DBUS_TYPE_INVALID);
      dbus_connection_send(conn, msg, nullptr);
      dbus_message_unref(msg);
    }
    dbus_connection_flush(conn);
  }

  ~Impl() {
    dequeue_all();
    if (conn) {
      dbus_connection_unref(conn);
      conn = nullptr;
    }
  }
#else
  bool ensure_service() { return false; }
  void dispatch(int) {}
  std::optional<std::uint32_t> queue(const std::vector<std::string>&,
                                      const std::vector<std::string>&,
                                      const char*) {
    return std::nullopt;
  }
  void dequeue_all() {}
  void deliver(XdgThumbnailReply reply) {
    std::vector<XdgThumbnailCallback> cbs;
    {
      std::lock_guard lock(mu);
      auto it = by_uri.find(reply.uri);
      if (it == by_uri.end()) {
        return;
      }
      cbs = std::move(it->second.callbacks);
      by_uri.erase(it);
    }
    for (auto& cb : cbs) {
      if (!cb) {
        continue;
      }
      XdgThumbnailReply copy = reply;
      auto fn = cb;
      executor.post([fn = std::move(fn), copy = std::move(copy)]() mutable {
        fn(std::move(copy));
      });
    }
  }
#endif
};

XdgThumbnailer::XdgThumbnailer(Executor executor)
    : impl_(std::make_unique<Impl>()) {
  impl_->executor = std::move(executor);
}

XdgThumbnailer::~XdgThumbnailer() = default;

bool XdgThumbnailer::service_available() const {
#if defined(THUMTOO_HAVE_DBUS) && THUMTOO_HAVE_DBUS
  return impl_->ensure_service();
#else
  return false;
#endif
}

void XdgThumbnailer::request(const std::filesystem::path& absolute_file,
                             std::string mime_type, XdgThumbnailFlavor flavor,
                             XdgThumbnailCallback cb, bool force) {
  request_many({absolute_file}, {std::move(mime_type)}, flavor, std::move(cb),
               force);
}

void XdgThumbnailer::request_many(
    const std::vector<std::filesystem::path>& files,
    const std::vector<std::string>& mime_types, XdgThumbnailFlavor flavor,
    XdgThumbnailCallback cb, bool force) {
  if (files.empty()) {
    return;
  }
  const char* flavor_name = xdg_thumbnail_flavor_name(flavor);
  std::vector<std::string> need_uris;
  std::vector<std::string> need_mimes;
  need_uris.reserve(files.size());
  need_mimes.reserve(files.size());

  for (std::size_t i = 0; i < files.size(); ++i) {
    const auto& file = files[i];
    const std::string uri = xdg_file_uri(file);
    if (force) {
      (void)xdg_thumbnail_remove_cache(file);
    } else if (auto hit = xdg_thumbnail_lookup(file, flavor)) {
      XdgThumbnailReply r;
      r.uri = uri;
      r.path = *hit;
      r.from_cache = true;
      if (cb) {
        impl_->executor.post([cb, r = std::move(r)]() mutable { cb(std::move(r)); });
      }
      continue;
    } else {
      // Stale file: remove so daemon rewrites
      const auto stale = xdg_thumbnail_cache_path(file, flavor);
      std::error_code ec;
      std::filesystem::remove(stale, ec);
    }
    need_uris.push_back(uri);
    need_mimes.push_back(i < mime_types.size() ? mime_types[i]
                                               : "application/octet-stream");
    if (cb) {
      std::lock_guard lock(impl_->mu);
      auto& pend = impl_->by_uri[uri];
      pend.flavor = flavor;
      pend.callbacks.push_back(cb);
    }
  }

  if (need_uris.empty()) {
    return;
  }

  if (!impl_->queue(need_uris, need_mimes, flavor_name)) {
    for (const auto& uri : need_uris) {
      XdgThumbnailReply r;
      r.uri = uri;
      r.error = xdg_thumbnail_dbus_built()
                    ? "thumbnail service unavailable"
                    : "thumtoo built without D-Bus (cache lookup only)";
      // Try any existing cache as last resort
      // (cannot recover path from uri without full decode — leave empty)
      impl_->deliver(std::move(r));
    }
  }
}

void XdgThumbnailer::cancel_all() {
  impl_->dequeue_all();
  std::lock_guard lock(impl_->mu);
  impl_->by_uri.clear();
}

void XdgThumbnailer::process_events(int timeout_ms) {
  impl_->dispatch(timeout_ms);
}

XdgThumbnailReply XdgThumbnailer::request_wait(
    const std::filesystem::path& absolute_file, std::string mime_type,
    XdgThumbnailFlavor flavor, int timeout_ms, bool force) {
  XdgThumbnailReply out;
  out.uri = xdg_file_uri(absolute_file);
  if (!force) {
    if (auto hit = xdg_thumbnail_lookup(absolute_file, flavor)) {
      out.path = *hit;
      out.from_cache = true;
      return out;
    }
  }

  std::mutex done_mu;
  bool done = false;
  XdgThumbnailReply result = out;
  request(
      absolute_file, std::move(mime_type), flavor,
      [&](XdgThumbnailReply r) {
        std::lock_guard lock(done_mu);
        result = std::move(r);
        done = true;
      },
      force);

  const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
  while (true) {
    {
      std::lock_guard lock(done_mu);
      if (done) {
        return result;
      }
    }
    if (std::chrono::steady_clock::now() >= deadline) {
      result.error = "timeout waiting for thumbnail";
      cancel_all();
      return result;
    }
    process_events(50);
  }
}

}  // namespace thumtoo
