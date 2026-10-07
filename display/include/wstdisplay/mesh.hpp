// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef HEADER_WSTDISPLAY_MESH_HPP
#define HEADER_WSTDISPLAY_MESH_HPP

#include <cstdint>
#include <span>
#include <vector>

#include "blend.hpp"
#include "fwd.hpp"
#include "handle.hpp"
#include "vertex.hpp"

namespace wstdisplay {

struct MeshVertex
{
  float x;
  float y;
  float z;
  float u;
  float v;
  float nx;
  float ny;
  float nz;
  PackedColor color;
};

static_assert(sizeof(MeshVertex) == 36);

/** An indexed triangle mesh with 3D positions and normals, e.g. a
    frame of a Sprite3D model, owned by a Device and created with
    Device::create_mesh(). Drawn with Canvas::draw(MeshId, transform)
    as a single draw call, never batched.

    The default shader draws texture * vertex color. A custom
    Material gets the same built-in names as the 2D shader plus
    a_normal and u_model (mat4, the model transform without the
    view/projection), a_position is a vec3.

    Positions are in pixels like everything else, z extends towards
    the viewer and is limited to [-4096, 4096].

    Changing vertices re-uploads them on the next draw, which is fine
    for keyframe animation. */
class Mesh final
{
public:
  Mesh();
  ~Mesh();

  void set_vertices(std::span<MeshVertex const> vertices);
  void set_indices(std::span<uint16_t const> indices);
  std::vector<MeshVertex> const& get_vertices() const { return m_vertices; }
  std::vector<uint16_t> const& get_indices() const { return m_indices; }

  /** The texture to draw with, null for white, not owned by the mesh */
  void set_texture(TextureId texture) { m_texture = texture; }
  TextureId get_texture() const { return m_texture; }

  /** A custom shader, null for the default one, not owned by the mesh */
  void set_material(MaterialId material) { m_material = material; }
  MaterialId get_material() const { return m_material; }

  void set_blend(Blend blend) { m_blend = blend; }
  Blend get_blend() const { return m_blend; }

  /** Use the depth buffer of the target, for meshes that overlap
      themselves. The target needs a depth buffer for this. */
  void set_depth_test(bool depth_test) { m_depth_test = depth_test; }
  bool get_depth_test() const { return m_depth_test; }

  int get_triangle_count() const { return static_cast<int>(m_indices.size() / 3); }

  /** Called by Renderer with the program already bound */
  void upload() const;
  void draw() const;

private:
  std::vector<MeshVertex> m_vertices;
  std::vector<uint16_t> m_indices;
  TextureId m_texture;
  MaterialId m_material;
  Blend m_blend;
  bool m_depth_test;

  mutable uint32_t m_vertex_buffer;
  mutable uint32_t m_index_buffer;
  mutable bool m_vertices_dirty;
  mutable bool m_indices_dirty;

public:
  Mesh(Mesh const&) = delete;
  Mesh& operator=(Mesh const&) = delete;
};

} // namespace wstdisplay

#endif

/* EOF */
