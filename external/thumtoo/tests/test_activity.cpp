// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "thumtoo/activity.hpp"

#include <cstdio>

static int fails = 0;
static void expect(bool ok, const char* msg) {
  if (!ok) {
    std::fprintf(stderr, "FAIL: %s\n", msg);
    ++fails;
  }
}

int main() {
  thumtoo::ActivityLedger ledger;
  const auto id = ledger.note_size_probe_queued("file:///tmp/a.jpg");
  ledger.note_size_probe_running(id);
  ledger.note_size_probe_finished(id, true);

  const auto aid =
      ledger.note_archive_member_running("/data/photos.zip", "img/001.jpg");
  ledger.note_archive_member_finished(aid, true);

  const auto sid = ledger.note_soft_queued("file:///tmp/b.jpg", 512);
  ledger.note_soft_running(sid);
  auto s = ledger.snapshot();
  expect(s.soft_running == 1, "soft running");
  ledger.note_soft_finished(sid, true);

  const auto tid = ledger.note_tile_queued("file:///tmp/c.jpg", 1, 2, 3);
  ledger.note_tile_running(tid);
  s = ledger.snapshot();
  expect(s.tile_running == 1, "tile running");
  expect(!s.running_tile_labels.empty(), "tile label");
  ledger.note_tile_finished(tid, true);
  s = ledger.snapshot();
  expect(s.tile_completed == 1, "tile done");
  expect(s.soft_completed == 1, "soft done");
  expect(s.size_probe_completed == 1, "size done");
  expect(s.archive_read_completed == 1, "archive done");

  // Supersede / cancel without Running must clear Queued (Client enqueue path).
  // Regression: finishing only via worker left tile_queued stuck → host "Working".
  {
    thumtoo::ActivityLedger led;
    const auto a = led.note_tile_queued("file:///tmp/d.jpg", 0, 1, 2);
    const auto b = led.note_tile_queued("file:///tmp/d.jpg", 0, 1, 2);
    auto snap = led.snapshot();
    expect(snap.tile_queued == 2, "two tile queued before finish");
    led.note_tile_finished(a, false);
    snap = led.snapshot();
    expect(snap.tile_queued == 1, "one remains after finish A");
    expect(snap.tile_running == 0, "none running after finish A");
    expect(snap.tile_completed == 1, "A counted completed");
    led.note_tile_finished(b, false);
    snap = led.snapshot();
    expect(snap.tile_queued == 0, "ledger clear after both finished");
    expect(snap.tile_running == 0, "still none running");
    expect(snap.tile_completed == 2, "both completed");
  }

  {
    thumtoo::ActivityLedger led;
    (void)led.note_tile_queued("file:///tmp/orphan.jpg", 0, 0, 0);
    (void)led.note_tile_queued("file:///tmp/orphan2.jpg", 1, 2, 3);
    auto snap = led.snapshot();
    expect(snap.tile_queued == 2, "two orphans before drop");
    led.drop_orphans();
    snap = led.snapshot();
    expect(snap.tile_queued == 0, "tile_queued 0 after drop_orphans");
    expect(snap.tile_running == 0, "tile_running 0 after drop_orphans");
  }

  if (fails) {
    std::fprintf(stderr, "%d failures\n", fails);
    return 1;
  }
  std::puts("ok");
  return 0;
}
