// Galapix - an image viewer for large image collections
// Copyright (C) 2008-2026 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include "thumtoo/thumtoo_callback_queue.hpp"

#include "galapix/viewer.hpp"

namespace galapix {

ThumtooCallbackQueue&
ThumtooCallbackQueue::instance()
{
  static ThumtooCallbackQueue queue;
  return queue;
}

thumtoo::Executor
ThumtooCallbackQueue::make_executor()
{
  return thumtoo::Executor([this](thumtoo::Executor::Fn fn) {
    m_queue.wait_and_push(std::move(fn));
    if (Viewer* v = Viewer::current()) {
      v->redraw();
    }
  });
}

void
ThumtooCallbackQueue::pump()
{
  std::function<void()> fn;
  while (m_queue.try_pop(fn)) {
    fn();
  }
}

} // namespace galapix

/* EOF */
