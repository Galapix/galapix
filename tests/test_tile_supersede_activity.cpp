// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * Regression: single-cell EnsureTiles supersede must finish ActivityLedger.
 * Without reply_cancelled_job, tile_queued grew while pending/inflight were 0
 * and hosts stayed on status=Working (tile=N/0).
 */

#include "thumtoo/activity.hpp"
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
  // Isolate global ledger from other process noise.
  global_activity_ledger().clear();

  const auto root = fs::temp_directory_path() / "thumtoo-tile-supersede" /
                    std::to_string(
                        std::chrono::steady_clock::now().time_since_epoch().count());
  fs::create_directories(root);
  const auto cache = root / "cache";

  // One worker is fine; we supersede before drain so queue pressure is the point.
  auto client = Client::open(cache, {}, /*worker_threads=*/1, cache);
  expect(!!client, "open client");
  if (!client) {
    return 1;
  }

  // Non-existent URI: workers miss quickly; supersede still runs in enqueue.
  const auto uri = file_uri_from_path(root / "missing-cell.jpg");

  std::atomic<int> miss_cb{0};
  auto on_miss = [&](std::string, int, int, int, std::optional<TileBlob>) {
    ++miss_cb;
  };

  // Flood same cell: each new request_tile supersedes the prior queued job.
  constexpr int kFlood = 32;
  for (int i = 0; i < kFlood; ++i) {
    client->request_tile(uri, /*scale=*/0, /*x=*/0, /*y=*/0, on_miss);
  }

  // After enqueue supersedes, at most one Queued/Running tile activity for this
  // cell should remain (plus any the worker already claimed).
  {
    const auto snap = client->activity_snapshot();
    const auto active = snap.tile_queued + snap.tile_running;
    expect(active <= 2, "supersede keeps at most a couple live tile activities");
    // The bug left active == kFlood (all Queued, never finished).
    expect(active < static_cast<std::size_t>(kFlood),
           "supersede must finish superseded tile activities");
  }

  client->drain();
  // Allow executor callbacks.
  for (int i = 0; i < 20; ++i) {
    client->drain();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    const auto snap = client->activity_snapshot();
    if (snap.tile_queued == 0 && snap.tile_running == 0) {
      break;
    }
  }

  {
    const auto snap = client->activity_snapshot();
    expect(snap.tile_queued == 0, "tile_queued settles to 0 after drain");
    expect(snap.tile_running == 0, "tile_running settles to 0 after drain");
  }

  // Second wave: prove repeated supersede does not accumulate forever.
  global_activity_ledger().clear();
  miss_cb = 0;
  for (int i = 0; i < kFlood; ++i) {
    client->request_tile(uri, 0, 1, 1, on_miss);
  }
  client->drain();
  for (int i = 0; i < 20; ++i) {
    client->drain();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    const auto snap = client->activity_snapshot();
    if (snap.tile_queued == 0 && snap.tile_running == 0) {
      break;
    }
  }
  {
    const auto snap = client->activity_snapshot();
    expect(snap.tile_queued == 0, "second wave tile_queued 0");
    expect(snap.tile_running == 0, "second wave tile_running 0");
    // completed should be at least the superseded finishes + worker results.
    expect(snap.tile_completed >= 1, "tile_completed advanced");
  }

  client.reset();
  fs::remove_all(root);

  if (g_fail) {
    std::cerr << g_fail << " failure(s)\n";
    return 1;
  }
  std::cout << "test_tile_supersede_activity: ok\n";
  return 0;
}
