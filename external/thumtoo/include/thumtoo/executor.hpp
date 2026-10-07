// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <functional>
#include <utility>

namespace thumtoo {

/// Caller-supplied marshaling hook (DESIGN API threading contract).
/// Library worker threads must not invoke app callbacks directly; they call
/// Executor::post so the host can run fn on the GUI/event-loop thread.
class Executor {
 public:
  using Fn = std::function<void()>;

  Executor() = default;
  explicit Executor(std::function<void(Fn)> post) : post_(std::move(post)) {}

  void post(Fn fn) const {
    if (post_) {
      post_(std::move(fn));
    } else {
      // Default: run inline (CLI / tests). GUI hosts must install a real post.
      fn();
    }
  }

 private:
  std::function<void(Fn)> post_;
};

}  // namespace thumtoo
