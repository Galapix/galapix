// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <wstdisplay/mesh.hpp>

#include <cstddef>

#include "gl.hpp"
#include <wstdisplay/shader_program.hpp>

namespace wstdisplay {

Mesh::Mesh() :
  m_vertices(),
  m_indices(),
  m_texture(),
  m_material(),
  m_blend(Blend::Alpha),
  m_depth_test(false),
  m_vertex_buffer(0),
  m_index_buffer(0),
  m_vertices_dirty(false),
  m_indices_dirty(false)
{
}

Mesh::~Mesh()
{
  if (m_vertex_buffer) {
    glDeleteBuffers(1, &m_vertex_buffer);
    glDeleteBuffers(1, &m_index_buffer);
  }
}

void
Mesh::set_vertices(std::span<MeshVertex const> vertices)
{
  m_vertices.assign(vertices.begin(), vertices.end());
  m_vertices_dirty = true;
}

void
Mesh::set_indices(std::span<uint16_t const> indices)
{
  m_indices.assign(indices.begin(), indices.end());
  m_indices_dirty = true;
}

void
Mesh::upload() const
{
  if (!m_vertex_buffer) {
    glGenBuffers(1, &m_vertex_buffer);
    glGenBuffers(1, &m_index_buffer);
    m_vertices_dirty = true;
    m_indices_dirty = true;
  }

  glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_index_buffer);

  if (m_vertices_dirty) {
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(m_vertices.size() * sizeof(MeshVertex)),
                 m_vertices.data(), GL_DYNAMIC_DRAW);
    m_vertices_dirty = false;
  }

  if (m_indices_dirty) {
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(m_indices.size() * sizeof(uint16_t)),
                 m_indices.data(), GL_STATIC_DRAW);
    m_indices_dirty = false;
  }
}

void
Mesh::draw() const
{
  if (m_indices.empty()) {
    return;
  }

  auto const offset = [](size_t member) {
    return reinterpret_cast<void const*>(member);
  };

  GLuint const position = static_cast<GLuint>(Attrib::Position);
  GLuint const texcoord = static_cast<GLuint>(Attrib::Texcoord);
  GLuint const color = static_cast<GLuint>(Attrib::Color);
  GLuint const normal = static_cast<GLuint>(Attrib::Normal);

  glEnableVertexAttribArray(position);
  glEnableVertexAttribArray(texcoord);
  glEnableVertexAttribArray(color);
  glEnableVertexAttribArray(normal);

  glVertexAttribPointer(position, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), offset(offsetof(MeshVertex, x)));
  glVertexAttribPointer(texcoord, 2, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), offset(offsetof(MeshVertex, u)));
  glVertexAttribPointer(normal, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), offset(offsetof(MeshVertex, nx)));
  glVertexAttribPointer(color, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(MeshVertex), offset(offsetof(MeshVertex, color)));

  glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_indices.size()), GL_UNSIGNED_SHORT, nullptr);
  WST_CHECK_GL("drawing mesh");
}

} // namespace wstdisplay

/* EOF */
