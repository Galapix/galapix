// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <wstdisplay/static_batch.hpp>

#include "gl.hpp"

namespace wstdisplay {

StaticBatch::StaticBatch() :
  m_batches(),
  m_vertex_buffer(0),
  m_index_buffer(0),
  m_dirty(false)
{
}

StaticBatch::~StaticBatch()
{
  if (m_vertex_buffer) {
    glDeleteBuffers(1, &m_vertex_buffer);
    glDeleteBuffers(1, &m_index_buffer);
  }
}

void
StaticBatch::build(Canvas const& canvas)
{
  build_batches(canvas, m_batches);
  m_batches.order.clear();
  m_batches.order.shrink_to_fit();
  m_dirty = true;
}

void
StaticBatch::clear()
{
  m_batches.clear();
  m_dirty = true;
}

void
StaticBatch::upload() const
{
  if (!m_dirty) {
    return;
  }

  if (!m_vertex_buffer) {
    glGenBuffers(1, &m_vertex_buffer);
    glGenBuffers(1, &m_index_buffer);
  }

  glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer);
  glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(m_batches.vertices.size() * sizeof(Vertex)),
               m_batches.vertices.data(), GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_index_buffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(m_batches.indices.size() * sizeof(uint16_t)),
               m_batches.indices.data(), GL_STATIC_DRAW);
  WST_CHECK_GL("uploading static batch");

  m_dirty = false;
}

} // namespace wstdisplay

/* EOF */
