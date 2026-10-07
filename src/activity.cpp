// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "thumtoo/activity.hpp"

#include <filesystem>
#include <string>

namespace thumtoo {
namespace {

std::string basename_of(const std::string& path) {
  try {
    return std::filesystem::path(path).filename().string();
  } catch (...) {
    return path;
  }
}

}  // namespace

ActivityLedger& global_activity_ledger() {
  static ActivityLedger ledger;
  return ledger;
}

std::uint64_t ActivityLedger::alloc_id_locked() {
  const std::uint64_t id = next_id_++;
  if (next_id_ == 0) {
    next_id_ = 1;
  }
  return id;
}

void ActivityLedger::cap_active_locked() {
  constexpr std::size_t kMaxActive = 4096;
  if (active_.size() > kMaxActive) {
    active_.erase(active_.begin(),
                  active_.begin() +
                      static_cast<std::ptrdiff_t>(active_.size() - kMaxActive));
  }
}

void ActivityLedger::set_phase_locked(std::uint64_t id, ActivityPhase phase) {
  for (auto& r : active_) {
    if (r.id == id) {
      r.phase = phase;
      return;
    }
  }
}

void ActivityLedger::finish_locked(std::uint64_t id,
                                   std::uint64_t* completed_counter) {
  for (auto it = active_.begin(); it != active_.end(); ++it) {
    if (it->id == id) {
      active_.erase(it);
      if (completed_counter) {
        ++(*completed_counter);
      }
      return;
    }
  }
  if (completed_counter) {
    ++(*completed_counter);
  }
}

std::uint64_t ActivityLedger::note_size_probe_queued(std::string uri) {
  std::lock_guard lock(mu_);
  const std::uint64_t id = alloc_id_locked();
  ActivityRecord r;
  r.id = id;
  r.kind = ActivityKind::SizeProbe;
  r.phase = ActivityPhase::Queued;
  r.uri = std::move(uri);
  active_.push_back(std::move(r));
  cap_active_locked();
  return id;
}

void ActivityLedger::note_size_probe_running(std::uint64_t id) {
  std::lock_guard lock(mu_);
  set_phase_locked(id, ActivityPhase::Running);
}

void ActivityLedger::note_size_probe_finished(std::uint64_t id, bool /*ok*/) {
  std::lock_guard lock(mu_);
  finish_locked(id, &size_probe_completed_);
}

std::uint64_t ActivityLedger::note_archive_member_running(
    std::string archive_path, std::string member) {
  std::lock_guard lock(mu_);
  const std::uint64_t id = alloc_id_locked();
  ActivityRecord r;
  r.id = id;
  r.kind = ActivityKind::ArchiveMemberRead;
  r.phase = ActivityPhase::Running;
  r.archive_root = archive_path;
  r.member_key = member;
  r.uri = archive_path + ":" + member;
  active_.push_back(std::move(r));
  cap_active_locked();
  return id;
}

void ActivityLedger::note_archive_member_finished(std::uint64_t id,
                                                   bool /*ok*/) {
  std::lock_guard lock(mu_);
  finish_locked(id, &archive_read_completed_);
}

std::uint64_t ActivityLedger::note_soft_queued(std::string uri, int max_edge) {
  std::lock_guard lock(mu_);
  const std::uint64_t id = alloc_id_locked();
  ActivityRecord r;
  r.id = id;
  r.kind = ActivityKind::SoftLadder;
  r.phase = ActivityPhase::Queued;
  r.uri = std::move(uri);
  r.detail = "edge=" + std::to_string(max_edge);
  active_.push_back(std::move(r));
  cap_active_locked();
  return id;
}

void ActivityLedger::note_soft_running(std::uint64_t id) {
  std::lock_guard lock(mu_);
  set_phase_locked(id, ActivityPhase::Running);
}

void ActivityLedger::note_soft_finished(std::uint64_t id, bool /*ok*/) {
  std::lock_guard lock(mu_);
  finish_locked(id, &soft_completed_);
}

std::uint64_t ActivityLedger::note_tile_queued(std::string uri, int scale, int x,
                                               int y) {
  std::lock_guard lock(mu_);
  const std::uint64_t id = alloc_id_locked();
  ActivityRecord r;
  r.id = id;
  r.kind = ActivityKind::TileCell;
  r.phase = ActivityPhase::Queued;
  r.uri = std::move(uri);
  r.detail = "s=" + std::to_string(scale) + " " + std::to_string(x) + "," +
             std::to_string(y);
  active_.push_back(std::move(r));
  cap_active_locked();
  return id;
}

void ActivityLedger::note_tile_running(std::uint64_t id) {
  std::lock_guard lock(mu_);
  set_phase_locked(id, ActivityPhase::Running);
}

void ActivityLedger::note_tile_finished(std::uint64_t id, bool /*ok*/) {
  std::lock_guard lock(mu_);
  finish_locked(id, &tile_completed_);
}

ActivitySnapshot ActivityLedger::snapshot() const {
  std::lock_guard lock(mu_);
  ActivitySnapshot s;
  s.size_probe_completed = size_probe_completed_;
  s.archive_read_completed = archive_read_completed_;
  s.soft_completed = soft_completed_;
  s.tile_completed = tile_completed_;
  for (const auto& r : active_) {
    switch (r.kind) {
      case ActivityKind::SizeProbe:
        if (r.phase == ActivityPhase::Running) {
          ++s.size_probe_running;
          if (s.running_size_probe_uris.size() < 8) {
            s.running_size_probe_uris.push_back(r.uri);
          }
        } else {
          ++s.size_probe_queued;
        }
        break;
      case ActivityKind::ArchiveMemberRead:
        ++s.archive_read_running;
        if (s.running_archive_labels.size() < 8) {
          s.running_archive_labels.push_back(basename_of(r.archive_root) + ":" +
                                             r.member_key);
        }
        break;
      case ActivityKind::SoftLadder:
        if (r.phase == ActivityPhase::Running) {
          ++s.soft_running;
          if (s.running_soft_uris.size() < 8) {
            s.running_soft_uris.push_back(r.uri);
          }
        } else {
          ++s.soft_queued;
        }
        break;
      case ActivityKind::TileCell:
        if (r.phase == ActivityPhase::Running) {
          ++s.tile_running;
          if (s.running_tile_labels.size() < 8) {
            s.running_tile_labels.push_back(basename_of(r.uri) + " " + r.detail);
          }
        } else {
          ++s.tile_queued;
        }
        break;
    }
  }
  return s;
}

void ActivityLedger::drop_orphans() {
  std::lock_guard lock(mu_);
  for (const auto& r : active_) {
    switch (r.kind) {
      case ActivityKind::SizeProbe:
        ++size_probe_completed_;
        break;
      case ActivityKind::ArchiveMemberRead:
        ++archive_read_completed_;
        break;
      case ActivityKind::SoftLadder:
        ++soft_completed_;
        break;
      case ActivityKind::TileCell:
        ++tile_completed_;
        break;
    }
  }
  active_.clear();
}

void ActivityLedger::clear() {
  std::lock_guard lock(mu_);
  active_.clear();
  size_probe_completed_ = 0;
  archive_read_completed_ = 0;
  soft_completed_ = 0;
  tile_completed_ = 0;
}

}  // namespace thumtoo
