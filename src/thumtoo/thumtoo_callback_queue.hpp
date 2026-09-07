// Galapix - an image viewer for large image collections
// Copyright (C) 2008-2026 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#ifndef HEADER_GALAPIX_THUMTOO_THUMTOO_CALLBACK_QUEUE_HPP
#define HEADER_GALAPIX_THUMTOO_THUMTOO_CALLBACK_QUEUE_HPP

#include "job/thread_message_queue2.hpp"

#include <functional>
#include <thumtoo/executor.hpp>

namespace galapix {

/** Optional queue for GUI-marshaled thumtoo callbacks.
 *
 *  Default make_executor() is thumtoo::Executor{} (inline on the Client worker).
 *  Tile JPEG decode and receive_tile queue push run off the GUI thread. OpenGL
 *  texture upload stays in ImageTileCache::process_queue on the main thread.
 *  pump() is a no-op with the default executor.
 */
class ThumtooCallbackQueue
{
public:
  static ThumtooCallbackQueue& instance();

  thumtoo::Executor make_executor();

  /** Run all queued callbacks on the calling thread (viewer/main). */
  void pump();

  /** Number of callbacks waiting for pump() (0 with default inline executor). */
  int size() const;

private:
  ThumtooCallbackQueue() = default;

  ThreadMessageQueue2<std::function<void()>> m_queue;

private:
  ThumtooCallbackQueue(ThumtooCallbackQueue const&) = delete;
  ThumtooCallbackQueue& operator=(ThumtooCallbackQueue const&) = delete;
};

} // namespace galapix

#endif

/* EOF */
