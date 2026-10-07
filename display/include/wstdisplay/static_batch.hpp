// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef HEADER_WSTDISPLAY_STATIC_BATCH_HPP
#define HEADER_WSTDISPLAY_STATIC_BATCH_HPP

#include <cstdint>

#include "batch.hpp"
#include "fwd.hpp"

namespace wstdisplay {

/** Geometry that is built once and kept on the GPU, for content that
    rarely changes, such as tilemap layers or chunks of destructible
    terrain. Owned by a Device and created with
    Device::create_static_batch(). Record the content into a Canvas,
    build() the batch from it and draw it with Canvas::draw(StaticBatchId)
    every frame, which costs one command instead of re-recording every
    tile.

    Draws using textures or materials that have been destroyed since
    are skipped. Clip rects, coordinate spaces as well as StaticBatch
    and Mesh draws of the recorded canvas are ignored. */
class StaticBatch final
{
public:
  StaticBatch();
  ~StaticBatch();

  /** Replace the content with that of \a canvas. The canvas can be
      cleared or reused afterwards. */
  void build(Canvas const& canvas);

  void clear();
  bool empty() const { return m_batches.batches.empty(); }

  BatchList const& get_batches() const { return m_batches; }

  /** Upload to the GPU if the content changed, called by Renderer */
  void upload() const;
  uint32_t get_vertex_buffer() const { return m_vertex_buffer; }
  uint32_t get_index_buffer() const { return m_index_buffer; }

private:
  BatchList m_batches;
  mutable uint32_t m_vertex_buffer;
  mutable uint32_t m_index_buffer;
  mutable bool m_dirty;

public:
  StaticBatch(StaticBatch const&) = delete;
  StaticBatch& operator=(StaticBatch const&) = delete;
};

} // namespace wstdisplay

#endif

/* EOF */
