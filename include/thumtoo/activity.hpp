// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace thumtoo {

/**
 * Live work snapshot for hosts (biltoo docs/ACTIVITY.md).
 * Phase 1–3: size probes, archive extracts, soft ladder, tile cells.
 */
enum class ActivityKind : int {
  SizeProbe = 0,
  ArchiveMemberRead = 1,
  SoftLadder = 2,
  TileCell = 3,
};

enum class ActivityPhase : int {
  Queued = 0,
  Running = 1,
};

struct ActivityRecord {
  std::uint64_t id = 0;
  ActivityKind kind = ActivityKind::SizeProbe;
  ActivityPhase phase = ActivityPhase::Queued;
  std::string uri;
  std::string archive_root;
  std::string member_key;
  std::string detail;  // e.g. "s=1 x=2 y=3" or "edge=512"
};

struct ActivitySnapshot {
  std::size_t size_probe_queued = 0;
  std::size_t size_probe_running = 0;
  std::uint64_t size_probe_completed = 0;
  std::vector<std::string> running_size_probe_uris;

  std::size_t archive_read_running = 0;
  std::uint64_t archive_read_completed = 0;
  std::vector<std::string> running_archive_labels;

  std::size_t soft_queued = 0;
  std::size_t soft_running = 0;
  std::uint64_t soft_completed = 0;
  std::vector<std::string> running_soft_uris;

  std::size_t tile_queued = 0;
  std::size_t tile_running = 0;
  std::uint64_t tile_completed = 0;
  std::vector<std::string> running_tile_labels;  // "uri leaf s=N"
};

class ActivityLedger {
 public:
  std::uint64_t note_size_probe_queued(std::string uri);
  void note_size_probe_running(std::uint64_t id);
  void note_size_probe_finished(std::uint64_t id, bool ok);

  std::uint64_t note_archive_member_running(std::string archive_path,
                                            std::string member);
  void note_archive_member_finished(std::uint64_t id, bool ok);

  std::uint64_t note_soft_queued(std::string uri, int max_edge);
  void note_soft_running(std::uint64_t id);
  void note_soft_finished(std::uint64_t id, bool ok);

  std::uint64_t note_tile_queued(std::string uri, int scale, int x, int y);
  void note_tile_running(std::uint64_t id);
  void note_tile_finished(std::uint64_t id, bool ok);

  [[nodiscard]] ActivitySnapshot snapshot() const;

  void clear();

  /**
   * Drop every active record and count them finished (false).
   * Used when the client queue is empty and inflight==0 so orphaned
   * note_*_queued entries cannot keep the host Performance badge on Working.
   */
  void drop_orphans();

 private:
  std::uint64_t alloc_id_locked();
  void set_phase_locked(std::uint64_t id, ActivityPhase phase);
  void finish_locked(std::uint64_t id, std::uint64_t* completed_counter);
  void cap_active_locked();

  mutable std::mutex mu_;
  std::uint64_t next_id_ = 1;
  std::uint64_t size_probe_completed_ = 0;
  std::uint64_t archive_read_completed_ = 0;
  std::uint64_t soft_completed_ = 0;
  std::uint64_t tile_completed_ = 0;
  std::vector<ActivityRecord> active_;
};

ActivityLedger& global_activity_ledger();

}  // namespace thumtoo
