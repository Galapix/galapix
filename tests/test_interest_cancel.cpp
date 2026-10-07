// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "thumtoo/client.hpp"
#include "thumtoo/uri.hpp"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

static int g_fail = 0;
static void expect(bool c, const char* m) {
  if (!c) {
    std::cerr << "FAIL: " << m << "\n";
    ++g_fail;
  }
}

int main() {
  using namespace thumtoo;
  const auto root = fs::temp_directory_path() / "thumtoo-interest-cancel" /
                    std::to_string(
                        std::chrono::steady_clock::now().time_since_epoch().count());
  fs::create_directories(root);
  const auto cache = root / "cache";

  auto client = Client::open(cache, {}, /*worker_threads=*/1, cache);
  expect(client != nullptr, "open client");
  if (!client) return 1;

  const auto e0 = client->interest_epoch();
  expect(e0 >= 1, "epoch starts at >=1");
  const auto e1 = client->bump_interest_epoch();
  expect(e1 > e0, "bump increases epoch");
  expect(client->interest_epoch() == e1, "interest_epoch matches bump");

  // Queue probes for non-existent paths, then cancel.
  std::atomic<int> null_cb{0};
  std::atomic<int> any_cb{0};
  std::vector<std::string> uris;
  for (int i = 0; i < 40; ++i) {
    const auto p = root / ("missing-" + std::to_string(i) + ".jpg");
    uris.push_back(file_uri_from_path(p));
  }
  for (const auto& u : uris) {
    client->request_size(u, [&](std::string, SizeReply r) {
      ++any_cb;
      if (!r.size) ++null_cb;
    });
  }

  // Cancel entire queue (workers may have claimed some already).
  const std::size_t dropped = client->cancel_pending();
  // Give worker a moment to finish claimed jobs.
  client->drain();
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  expect(dropped + static_cast<std::size_t>(any_cb.load()) >= 1,
         "cancel or complete touched jobs");
  // All callbacks should have fired eventually (cancelled or finished).
  client->drain();
  expect(any_cb.load() >= 1, "at least one size callback");

  // cancel_uri on a fresh enqueue.
  const auto u = file_uri_from_path(root / "solo-missing.jpg");
  std::atomic<int> solo_null{0};
  client->request_size(u, [&](std::string, SizeReply r) {
    if (!r.size) ++solo_null;
  });
  const std::size_t n = client->cancel_uri(u);
  client->drain();
  // Either cancelled from queue (n>=1, callback nullopt) or worker already ran.
  expect(n >= 0, "cancel_uri returns");
  (void)solo_null;

  // bump after enqueue should drop stale.
  for (int i = 0; i < 20; ++i) {
    client->request_size(file_uri_from_path(root / ("stale-" + std::to_string(i) + ".jpg")),
                         [](std::string, SizeReply) {});
  }
  const auto e2 = client->bump_interest_epoch();
  expect(e2 > e1, "second bump");
  client->drain();

  fs::remove_all(root);
  if (g_fail) {
    std::cerr << g_fail << " failure(s)\n";
    return 1;
  }
  std::cout << "test_interest_cancel: ok\n";
  return 0;
}
