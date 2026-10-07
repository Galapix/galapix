// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "thumtoo/network.hpp"

#include <string>

#if defined(THUMTOO_HAVE_CURL)
#include <curl/curl.h>
#endif

namespace thumtoo {
namespace {

#if defined(THUMTOO_HAVE_CURL)
struct WriteCtx {
  std::vector<std::uint8_t>* out = nullptr;
  std::uint64_t max_bytes = 0;
  bool overflow = false;
};

std::size_t write_cb(char* ptr, std::size_t size, std::size_t nmemb, void* userdata) {
  auto* ctx = static_cast<WriteCtx*>(userdata);
  const std::size_t n = size * nmemb;
  if (ctx->overflow) return 0;
  if (ctx->out->size() + n > ctx->max_bytes) {
    ctx->overflow = true;
    return 0;  // abort transfer
  }
  const auto* bytes = reinterpret_cast<const std::uint8_t*>(ptr);
  ctx->out->insert(ctx->out->end(), bytes, bytes + n);
  return n;
}
#endif

}  // namespace

bool http_fetch_available() {
#if defined(THUMTOO_HAVE_CURL)
  return true;
#else
  return false;
#endif
}

std::optional<std::vector<std::uint8_t>> http_get_bytes(std::string_view url,
                                                        std::uint64_t max_bytes) {
#if !defined(THUMTOO_HAVE_CURL)
  (void)url;
  (void)max_bytes;
  return std::nullopt;
#else
  if (url.empty() || max_bytes == 0) return std::nullopt;
  if (!(url.starts_with("http://") || url.starts_with("https://"))) {
    return std::nullopt;
  }

  CURL* curl = curl_easy_init();
  if (!curl) return std::nullopt;

  std::vector<std::uint8_t> body;
  body.reserve(std::min<std::uint64_t>(max_bytes, 64 * 1024));
  WriteCtx ctx{&body, max_bytes, false};

  const std::string url_str(url);
  curl_easy_setopt(curl, CURLOPT_URL, url_str.c_str());
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 8L);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "thumtoo/0.1");
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ctx);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15L);
  // Cap download size at protocol level when possible.
  curl_easy_setopt(curl, CURLOPT_MAXFILESIZE_LARGE,
                   static_cast<curl_off_t>(max_bytes));

  const CURLcode rc = curl_easy_perform(curl);
  long http_code = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
  curl_easy_cleanup(curl);

  if (rc != CURLE_OK || ctx.overflow) return std::nullopt;
  if (http_code < 200 || http_code >= 300) return std::nullopt;
  if (body.empty()) return std::nullopt;
  return body;
#endif
}

}  // namespace thumtoo
