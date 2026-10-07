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
  // Default / inline Executor: Client worker threads run tile callbacks on the
  // worker itself. Tile decode to RGBA8 (thumtoo::lod::ClientTileBackend) and
  // the TileLoader inbox push stay off the GUI thread; the scheduler wake only
  // requests a redraw — nothing here may touch OpenGL.
  //
  // Size probes during create() complete inside Client::drain() without a
  // main-thread pump.
  return thumtoo::Executor{};
}

void
ThumtooCallbackQueue::pump()
{
  // Reserved for hosts that install a GUI-marshaling executor. With the default
  // inline executor this is a no-op.
  std::function<void()> fn;
  while (m_queue.try_pop(fn)) {
    fn();
  }
}

int
ThumtooCallbackQueue::size() const
{
  return m_queue.size();
}

} // namespace galapix

/* EOF */
