// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "thumtoo/client.hpp"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>

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
  const auto root = fs::temp_directory_path() / "thumtoo-queue-stats" /
                    std::to_string(
                        std::chrono::steady_clock::now().time_since_epoch().count());
  fs::create_directories(root);
  const auto cache = root / "cache";

  auto client = Client::open(cache, {}, 1, cache);
  expect(!!client, "open");
  if (!client) return 1;

  auto s0 = client->queue_stats();
  expect(s0.pending == 0, "pending starts 0");
  expect(s0.inflight == 0, "inflight starts 0");
  expect(s0.focus_full_inflight == 0, "focus_full starts 0");
  expect(s0.interest_epoch == client->interest_epoch(), "epoch matches");

  const auto e1 = client->bump_interest_epoch();
  auto s1 = client->queue_stats();
  expect(s1.interest_epoch == e1, "stats epoch after bump");
  expect(s1.interest_epoch > s0.interest_epoch, "epoch increased");

  client->drain();
  fs::remove_all(root);

  if (g_fail) {
    std::cerr << g_fail << " failure(s)\n";
    return 1;
  }
  std::cout << "test_queue_stats: ok\n";
  return 0;
}
