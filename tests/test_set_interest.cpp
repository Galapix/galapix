// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "thumtoo/client.hpp"
#include "thumtoo/constants.hpp"
#include "thumtoo/types.hpp"
#include "thumtoo/uri.hpp"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>
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
  const auto root = fs::temp_directory_path() / "thumtoo-set-interest" /
                    std::to_string(
                        std::chrono::steady_clock::now().time_since_epoch().count());
  fs::create_directories(root);
  const auto cache = root / "cache";

  auto client = Client::open(cache, {}, 1, cache);
  expect(!!client, "open");
  if (!client) return 1;

  const auto e0 = client->interest_epoch();
  std::vector<InterestItem> items;
  InterestItem a;
  a.uri = file_uri_from_path(root / "a.jpg");
  a.target_long_edge = 512;
  a.role = InterestRole::Near;
  items.push_back(a);
  InterestItem b;
  b.uri = file_uri_from_path(root / "b.jpg");
  b.target_long_edge = 1024;
  b.role = InterestRole::Primary;
  items.push_back(b);
  // Duplicate uri with lower role — should keep Primary.
  InterestItem b2 = b;
  b2.role = InterestRole::Speculative;
  b2.target_long_edge = 128;
  items.push_back(b2);

  const auto e1 = client->set_interest(std::move(items));
  expect(e1 > e0, "set_interest bumps epoch");
  expect(client->interest_epoch() == e1, "epoch matches");

  // Empty snapshot still bumps.
  const auto e2 = client->set_interest({});
  expect(e2 > e1, "empty set_interest bumps");

  client->drain();
  fs::remove_all(root);

  if (g_fail) {
    std::cerr << g_fail << " failure(s)\n";
    return 1;
  }
  std::cout << "test_set_interest: ok\n";
  return 0;
}
