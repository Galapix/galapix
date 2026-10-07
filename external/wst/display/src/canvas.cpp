// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <wstdisplay/canvas.hpp>

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <stdexcept>

#include <glm/gtc/constants.hpp>
#include <geom/offset.hpp>
#include <geom/quad.hpp>

#include <wstdisplay/font/font.hpp>
#include <wstdisplay/surface.hpp>

namespace wstdisplay {

namespace {

constexpr std::array<uint16_t, 6> quad_indices = { 0, 1, 2, 0, 2, 3 };

glm::vec2 to_vec(geom::fpoint const& p)
{
  return glm::vec2(p.x(), p.y());
}

float deg2rad(float degrees)
{
  return degrees * glm::pi<float>() / 180.0f;
}

} // namespace

Canvas::Canvas() :
  m_commands(),
  m_vertices(),
  m_indices(),
  m_clip_rects{ClipRect{geom::frect(), Space::World}},
  m_objects(),
  m_state(),
  m_state_stack(),
  m_unordered_layers(),
  m_points(),
  m_scratch_indices()
{
}

void
Canvas::clear()
{
  m_commands.clear();
  m_vertices.clear();
  m_indices.clear();
  m_clip_rects.resize(1);
  m_objects.clear();

  m_state = State();
  m_state_stack.clear();
}

/*{ State */

void
Canvas::save()
{
  m_state_stack.push_back(m_state);
}

void
Canvas::restore()
{
  if (m_state_stack.empty()) {
    throw std::logic_error("Canvas::restore: no matching save()");
  }
  m_state = m_state_stack.back();
  m_state_stack.pop_back();
}

/*} */

/*{ Transform */

void
Canvas::reset_transform()
{
  m_state.transform = glm::mat3(1.0f);
  m_state.identity = true;
}

void
Canvas::set_transform(glm::mat3 const& transform)
{
  m_state.transform = transform;
  m_state.identity = (transform == glm::mat3(1.0f));
}

void
Canvas::mult_transform(glm::mat3 const& transform)
{
  set_transform(m_state.transform * transform);
}

void
Canvas::translate(float x, float y)
{
  glm::mat3 m(1.0f);
  m[2] = glm::vec3(x, y, 1.0f);
  mult_transform(m);
}

void
Canvas::scale(float x, float y)
{
  glm::mat3 m(1.0f);
  m[0][0] = x;
  m[1][1] = y;
  mult_transform(m);
}

void
Canvas::rotate(float degrees)
{
  float const c = std::cos(deg2rad(degrees));
  float const s = std::sin(deg2rad(degrees));
  glm::mat3 m(1.0f);
  m[0] = glm::vec3(c, s, 0.0f);
  m[1] = glm::vec3(-s, c, 0.0f);
  mult_transform(m);
}

glm::vec2
Canvas::apply(float x, float y) const
{
  if (m_state.identity) {
    return glm::vec2(x, y);
  } else {
    glm::vec3 const p = m_state.transform * glm::vec3(x, y, 1.0f);
    return glm::vec2(p.x, p.y);
  }
}

void
Canvas::transform_vertices(Vertex* vertices, size_t count) const
{
  if (m_state.identity) {
    return;
  }

  glm::mat3 const& m = m_state.transform;
  for (size_t i = 0; i < count; ++i) {
    float const x = vertices[i].x;
    float const y = vertices[i].y;
    vertices[i].x = m[0][0] * x + m[1][0] * y + m[2][0];
    vertices[i].y = m[0][1] * x + m[1][1] * y + m[2][1];
  }
}

/*} */

/*{ Clipping */

void
Canvas::clip(geom::frect const& rect)
{
  glm::vec2 const corners[] = {
    apply(rect.left(), rect.top()),
    apply(rect.right(), rect.top()),
    apply(rect.right(), rect.bottom()),
    apply(rect.left(), rect.bottom())
  };

  float left = corners[0].x;
  float right = corners[0].x;
  float top = corners[0].y;
  float bottom = corners[0].y;
  for (auto const& p : corners) {
    left = std::min(left, p.x);
    right = std::max(right, p.x);
    top = std::min(top, p.y);
    bottom = std::max(bottom, p.y);
  }

  geom::frect clip_rect(left, top, right, bottom);

  // intersect with the parent clip if both are in the same space
  uint16_t const parent = m_state.clip;
  if (parent != 0 && m_clip_rects[parent].space == m_state.space) {
    geom::frect const& prect = m_clip_rects[parent].rect;
    float const l = std::max(clip_rect.left(), prect.left());
    float const t = std::max(clip_rect.top(), prect.top());
    clip_rect = geom::frect(l, t,
                            std::max(l, std::min(clip_rect.right(), prect.right())),
                            std::max(t, std::min(clip_rect.bottom(), prect.bottom())));
  }

  if (m_clip_rects.size() >= 0xffff) {
    throw std::runtime_error("Canvas::clip: too many clip rects");
  }

  m_clip_rects.push_back(ClipRect{clip_rect, m_state.space});
  m_state.clip = static_cast<uint16_t>(m_clip_rects.size() - 1);
}

/*} */

/*{ Layers */

void
Canvas::set_layer_unordered(float z, bool unordered)
{
  auto it = std::lower_bound(m_unordered_layers.begin(), m_unordered_layers.end(), z);
  bool const present = (it != m_unordered_layers.end() && *it == z);
  if (unordered && !present) {
    m_unordered_layers.insert(it, z);
  } else if (!unordered && present) {
    m_unordered_layers.erase(it);
  }
}

bool
Canvas::is_layer_unordered(float z) const
{
  return std::binary_search(m_unordered_layers.begin(), m_unordered_layers.end(), z);
}

/*} */

/*{ Command recording */

Vertex*
Canvas::add(TextureId texture, uint32_t vertex_count, std::span<uint16_t const> indices)
{
  if (vertex_count > max_command_vertices) {
    throw std::length_error("Canvas: too many vertices in a single draw");
  }

  uint32_t base = 0;
  if (!m_commands.empty() &&
      m_commands.back().type == CommandType::Geometry &&
      m_commands.back().texture == texture &&
      m_commands.back().material == m_state.material &&
      m_commands.back().blend == m_state.blend &&
      m_commands.back().z == m_state.z &&
      m_commands.back().clip == m_state.clip &&
      m_commands.back().space == m_state.space &&
      m_commands.back().vertex_count + vertex_count <= max_command_vertices)
  {
    Command& cmd = m_commands.back();
    base = cmd.vertex_count;
    cmd.vertex_count += vertex_count;
    cmd.index_count += static_cast<uint32_t>(indices.size());
  }
  else
  {
    m_commands.push_back(Command{
        .z = m_state.z,
        .first_vertex = static_cast<uint32_t>(m_vertices.size()),
        .vertex_count = vertex_count,
        .first_index = static_cast<uint32_t>(m_indices.size()),
        .index_count = static_cast<uint32_t>(indices.size()),
        .texture = texture,
        .material = m_state.material,
        .object = 0,
        .clip = m_state.clip,
        .blend = m_state.blend,
        .space = m_state.space,
        .type = CommandType::Geometry
      });
  }

  for (uint16_t const idx : indices) {
    assert(idx < vertex_count);
    m_indices.push_back(static_cast<uint16_t>(base + idx));
  }

  size_t const offset = m_vertices.size();
  m_vertices.resize(offset + vertex_count);
  return m_vertices.data() + offset;
}

Vertex*
Canvas::add_quads(TextureId texture, uint32_t quad_count)
{
  if (quad_count == 1) {
    return add(texture, 4, quad_indices);
  }

  // build the index pattern in place, avoids a temporary
  uint32_t const vertex_count = quad_count * 4;
  if (vertex_count > max_command_vertices) {
    throw std::length_error("Canvas: too many quads in a single draw");
  }

  size_t const first_index = m_indices.size();
  Vertex* vertices = add(texture, vertex_count, {});
  uint32_t const base = static_cast<uint32_t>(m_commands.back().vertex_count - vertex_count);

  m_indices.reserve(first_index + quad_count * 6);
  for (uint32_t q = 0; q < quad_count; ++q) {
    for (uint16_t const idx : quad_indices) {
      m_indices.push_back(static_cast<uint16_t>(base + q * 4 + idx));
    }
  }
  m_commands.back().index_count += quad_count * 6;
  return vertices;
}

void
Canvas::add_object(CommandType type, ObjectDraw const& object)
{
  glm::mat3 const& t = m_state.transform;
  glm::mat4 canvas_matrix(1.0f);
  canvas_matrix[0] = glm::vec4(t[0][0], t[0][1], 0.0f, 0.0f);
  canvas_matrix[1] = glm::vec4(t[1][0], t[1][1], 0.0f, 0.0f);
  canvas_matrix[3] = glm::vec4(t[2][0], t[2][1], 0.0f, 1.0f);

  m_objects.push_back(ObjectDraw{
      .static_batch = object.static_batch,
      .mesh = object.mesh,
      .matrix = canvas_matrix * object.matrix
    });

  m_commands.push_back(Command{
      .z = m_state.z,
      .first_vertex = 0,
      .vertex_count = 0,
      .first_index = 0,
      .index_count = 0,
      .texture = {},
      .material = {},
      .object = static_cast<uint32_t>(m_objects.size() - 1),
      .clip = m_state.clip,
      .blend = m_state.blend,
      .space = m_state.space,
      .type = type
    });
}

void
Canvas::add_rect(TextureId texture, geom::frect const& rect, geom::frect const& uv, PackedColor color)
{
  Vertex* v = add_quads(texture, 1);
  v[0] = Vertex{rect.left(), rect.top(), uv.left(), uv.top(), color};
  v[1] = Vertex{rect.right(), rect.top(), uv.right(), uv.top(), color};
  v[2] = Vertex{rect.right(), rect.bottom(), uv.right(), uv.bottom(), color};
  v[3] = Vertex{rect.left(), rect.bottom(), uv.left(), uv.bottom(), color};
  transform_vertices(v, 4);
}

/*} */

/*{ Surfaces */

void
Canvas::draw(Surface const& surface, geom::fpoint const& pos)
{
  if (!surface) {
    return;
  }

  add_rect(surface.get_texture(), geom::frect(pos, surface.get_size()), surface.get_uv(), packed_white);
}

void
Canvas::draw(Surface const& surface, DrawParams const& params)
{
  if (!surface) {
    return;
  }

  geom::frect const& uv = surface.get_uv();

  float u0 = uv.left();
  float u1 = uv.right();
  float v0 = uv.top();
  float v1 = uv.bottom();
  if (params.hflip) { std::swap(u0, u1); }
  if (params.vflip) { std::swap(v0, v1); }

  float const w = surface.get_width() * params.scale.x;
  float const h = surface.get_height() * params.scale.y;
  geom::foffset const anchor = geom::anchor_offset(geom::fsize(w, h), params.anchor) +
    geom::foffset(params.offset.x() * params.scale.x, params.offset.y() * params.scale.y);

  glm::vec2 corners[4] = {
    {anchor.x(), anchor.y()},
    {anchor.x() + w, anchor.y()},
    {anchor.x() + w, anchor.y() + h},
    {anchor.x(), anchor.y() + h}
  };

  if (params.angle != 0.0f) {
    float const c = std::cos(deg2rad(params.angle));
    float const s = std::sin(deg2rad(params.angle));
    for (auto& p : corners) {
      p = glm::vec2(c * p.x - s * p.y, s * p.x + c * p.y);
    }
  }

  PackedColor const color = pack_color(params.color);
  PackedColor const effect = pack_color(params.effect);
  float const x = params.pos.x();
  float const y = params.pos.y();

  Vertex* v = add_quads(surface.get_texture(), 1);
  v[0] = Vertex{x + corners[0].x, y + corners[0].y, u0, v0, color, effect};
  v[1] = Vertex{x + corners[1].x, y + corners[1].y, u1, v0, color, effect};
  v[2] = Vertex{x + corners[2].x, y + corners[2].y, u1, v1, color, effect};
  v[3] = Vertex{x + corners[3].x, y + corners[3].y, u0, v1, color, effect};
  transform_vertices(v, 4);
}

void
Canvas::draw(Surface const& surface, geom::frect const& dstrect, surf::Color const& color)
{
  if (!surface) {
    return;
  }

  add_rect(surface.get_texture(), dstrect, surface.get_uv(), pack_color(color));
}

void
Canvas::fill_pattern(Surface const& pattern, geom::frect const& rect, geom::foffset const& offset,
                     surf::Color const& color)
{
  if (!pattern) {
    return;
  }

  assert(pattern.get_uv() == geom::frect(0.0f, 0.0f, 1.0f, 1.0f) &&
         "Canvas::fill_pattern: the pattern must cover its whole texture");

  float const tw = pattern.get_width();
  float const th = pattern.get_height();
  float const u0 = (rect.left() - offset.x()) / tw;
  float const v0 = (rect.top() - offset.y()) / th;
  geom::frect const uv(u0, v0, u0 + rect.width() / tw, v0 + rect.height() / th);

  add_rect(pattern.get_texture(), rect, uv, pack_color(color));
}

/*} */

/*{ Shapes */

void
Canvas::fill_screen(surf::Color const& color)
{
  Space const old_space = m_state.space;
  m_state.space = Space::Clip;

  PackedColor const pc = pack_color(color);
  Vertex* v = add_quads({}, 1);
  v[0] = Vertex{-1.0f, -1.0f, 0.0f, 0.0f, pc};
  v[1] = Vertex{1.0f, -1.0f, 0.0f, 0.0f, pc};
  v[2] = Vertex{1.0f, 1.0f, 0.0f, 0.0f, pc};
  v[3] = Vertex{-1.0f, 1.0f, 0.0f, 0.0f, pc};

  m_state.space = old_space;
}

void
Canvas::fill_rect(geom::frect const& rect, surf::Color const& color)
{
  add_rect({}, rect, geom::frect(), pack_color(color));
}

void
Canvas::draw_rect(geom::frect const& rect, surf::Color const& color, float width)
{
  // four non-overlapping bars inside the rect, pixel exact for
  // integer coordinates and width 1
  float const w = std::min(width, std::min(rect.width(), rect.height()) / 2.0f);
  PackedColor const pc = pack_color(color);

  geom::frect const bars[] = {
    geom::frect(rect.left(), rect.top(), rect.right(), rect.top() + w),
    geom::frect(rect.left(), rect.bottom() - w, rect.right(), rect.bottom()),
    geom::frect(rect.left(), rect.top() + w, rect.left() + w, rect.bottom() - w),
    geom::frect(rect.right() - w, rect.top() + w, rect.right(), rect.bottom() - w)
  };

  Vertex* v = add_quads({}, 4);
  for (auto const& bar : bars) {
    *v++ = Vertex{bar.left(), bar.top(), 0.0f, 0.0f, pc};
    *v++ = Vertex{bar.right(), bar.top(), 0.0f, 0.0f, pc};
    *v++ = Vertex{bar.right(), bar.bottom(), 0.0f, 0.0f, pc};
    *v++ = Vertex{bar.left(), bar.bottom(), 0.0f, 0.0f, pc};
  }
  transform_vertices(v - 16, 16);
}

int
Canvas::segments_for(float radius, float degrees) const
{
  // keep the distance between the arc and its chords below a quarter
  // pixel, at the scale of the canvas transform
  constexpr float max_error = 0.25f;
  glm::mat3 const& m = m_state.transform;
  float const scale = std::sqrt(std::abs(m[0][0] * m[1][1] - m[1][0] * m[0][1]));
  float const r = std::max(std::abs(radius) * scale, max_error);
  float const step = 2.0f * std::acos(1.0f - max_error / r);
  int const full = std::clamp(static_cast<int>(std::ceil(2.0f * glm::pi<float>() / step)), 8, 256);
  return std::max(1, static_cast<int>(std::ceil(static_cast<float>(full) * std::abs(degrees) / 360.0f)));
}

namespace {

void rounded_rect_points(geom::frect const& rect, float radius, int segments, std::vector<glm::vec2>& points)
{
  points.clear();

  if (radius <= 0.0f) {
    points.emplace_back(rect.left(), rect.top());
    points.emplace_back(rect.right(), rect.top());
    points.emplace_back(rect.right(), rect.bottom());
    points.emplace_back(rect.left(), rect.bottom());
    return;
  }

  struct Corner { float cx; float cy; float start; };
  Corner const corners[] = {
    { rect.right() - radius, rect.top() + radius, -90.0f },
    { rect.right() - radius, rect.bottom() - radius, 0.0f },
    { rect.left() + radius, rect.bottom() - radius, 90.0f },
    { rect.left() + radius, rect.top() + radius, 180.0f },
  };

  for (auto const& corner : corners) {
    for (int i = 0; i <= segments; ++i) {
      float const a = deg2rad(corner.start + 90.0f * static_cast<float>(i) / static_cast<float>(segments));
      points.emplace_back(corner.cx + std::cos(a) * radius,
                          corner.cy + std::sin(a) * radius);
    }
  }
}

void arc_points(geom::fpoint const& center, float radius, float start, float end, int segments,
                std::vector<glm::vec2>& points)
{
  points.clear();
  for (int i = 0; i <= segments; ++i) {
    float const a = deg2rad(start + (end - start) * static_cast<float>(i) / static_cast<float>(segments));
    points.emplace_back(center.x() + std::cos(a) * radius,
                        center.y() + std::sin(a) * radius);
  }
}

} // namespace

void
Canvas::add_polygon_fill(std::span<glm::vec2 const> points, PackedColor color)
{
  // convex polygons only, drawn as a fan
  if (points.size() < 3) {
    return;
  }

  uint32_t const n = static_cast<uint32_t>(points.size());
  m_scratch_indices.clear();
  for (uint32_t i = 1; i + 1 < n; ++i) {
    m_scratch_indices.push_back(0);
    m_scratch_indices.push_back(static_cast<uint16_t>(i));
    m_scratch_indices.push_back(static_cast<uint16_t>(i + 1));
  }

  Vertex* v = add({}, n, m_scratch_indices);
  for (uint32_t i = 0; i < n; ++i) {
    v[i] = Vertex{points[i].x, points[i].y, 0.0f, 0.0f, color};
  }
  transform_vertices(v, n);
}

void
Canvas::add_line_strip(std::span<glm::vec2 const> points, bool loop, PackedColor color, float width)
{
  size_t const n = points.size();
  if (n < 2) {
    return;
  }

  float const hw = width / 2.0f;
  auto normal = [&](size_t a, size_t b) {
    glm::vec2 d = points[b] - points[a];
    float const len = glm::length(d);
    if (len <= 0.0f) {
      return glm::vec2(0.0f, 0.0f);
    }
    d /= len;
    return glm::vec2(-d.y, d.x);
  };

  uint32_t const vertex_count = static_cast<uint32_t>(n * 2);
  size_t const segment_count = loop ? n : n - 1;

  m_scratch_indices.clear();
  for (size_t i = 0; i < segment_count; ++i) {
    uint16_t const a = static_cast<uint16_t>(i * 2);
    uint16_t const b = static_cast<uint16_t>(((i + 1) % n) * 2);
    m_scratch_indices.insert(m_scratch_indices.end(),
                             {a, b, static_cast<uint16_t>(b + 1),
                              a, static_cast<uint16_t>(b + 1), static_cast<uint16_t>(a + 1)});
  }

  Vertex* v = add({}, vertex_count, m_scratch_indices);
  for (size_t i = 0; i < n; ++i) {
    glm::vec2 offset;
    bool const has_prev = loop || i > 0;
    bool const has_next = loop || i + 1 < n;
    if (has_prev && has_next) {
      // miter join
      glm::vec2 const n1 = normal((i + n - 1) % n, i);
      glm::vec2 const n2 = normal(i, (i + 1) % n);
      glm::vec2 m = n1 + n2;
      float const len = glm::length(m);
      if (len < 0.0001f) {
        offset = n1 * hw;
      } else {
        m /= len;
        float const d = std::max(glm::dot(m, n1), 0.25f); // miter limit
        offset = m * (hw / d);
      }
    } else if (has_next) {
      offset = normal(i, i + 1) * hw;
    } else {
      offset = normal(i - 1, i) * hw;
    }

    glm::vec2 const a = points[i] + offset;
    glm::vec2 const b = points[i] - offset;
    v[i * 2 + 0] = Vertex{a.x, a.y, 0.0f, 0.0f, color};
    v[i * 2 + 1] = Vertex{b.x, b.y, 0.0f, 0.0f, color};
  }
  transform_vertices(v, vertex_count);
}

void
Canvas::fill_rounded_rect(geom::frect const& rect, float radius, surf::Color const& color)
{
  radius = std::min(radius, std::min(rect.width(), rect.height()) / 2.0f);
  rounded_rect_points(rect, radius, segments_for(radius, 90.0f), m_points);
  add_polygon_fill(m_points, pack_color(color));
}

void
Canvas::draw_rounded_rect(geom::frect const& rect, float radius, surf::Color const& color, float width)
{
  // inset by half the width so the outline stays inside rect
  float const hw = width / 2.0f;
  geom::frect const inner(rect.left() + hw, rect.top() + hw, rect.right() - hw, rect.bottom() - hw);
  radius = std::min(std::max(0.0f, radius - hw), std::min(inner.width(), inner.height()) / 2.0f);
  rounded_rect_points(inner, radius, segments_for(radius, 90.0f), m_points);
  add_line_strip(m_points, true, pack_color(color), width);
}

void
Canvas::fill_quad(geom::fquad const& quad, surf::Color const& color)
{
  glm::vec2 const points[] = { to_vec(quad.p1), to_vec(quad.p2), to_vec(quad.p3), to_vec(quad.p4) };
  add_polygon_fill(points, pack_color(color));
}

void
Canvas::draw_quad(geom::fquad const& quad, surf::Color const& color, float width)
{
  glm::vec2 const points[] = { to_vec(quad.p1), to_vec(quad.p2), to_vec(quad.p3), to_vec(quad.p4) };
  add_line_strip(points, true, pack_color(color), width);
}

void
Canvas::fill_triangle(geom::fpoint const& p1, geom::fpoint const& p2, geom::fpoint const& p3,
                      surf::Color const& color)
{
  glm::vec2 const points[] = { to_vec(p1), to_vec(p2), to_vec(p3) };
  add_polygon_fill(points, pack_color(color));
}

void
Canvas::fill_polygon(std::span<geom::fpoint const> points, surf::Color const& color)
{
  m_points.clear();
  for (auto const& p : points) {
    m_points.push_back(to_vec(p));
  }
  add_polygon_fill(m_points, pack_color(color));
}

void
Canvas::draw_polygon(std::span<geom::fpoint const> points, surf::Color const& color, float width)
{
  m_points.clear();
  for (auto const& p : points) {
    m_points.push_back(to_vec(p));
  }
  add_line_strip(m_points, true, pack_color(color), width);
}

void
Canvas::draw_line(geom::fpoint const& p1, geom::fpoint const& p2, surf::Color const& color, float width)
{
  glm::vec2 const points[] = { to_vec(p1), to_vec(p2) };
  add_line_strip(points, false, pack_color(color), width);
}

void
Canvas::draw_polyline(std::span<geom::fpoint const> points, surf::Color const& color, float width)
{
  m_points.clear();
  for (auto const& p : points) {
    m_points.push_back(to_vec(p));
  }
  add_line_strip(m_points, false, pack_color(color), width);
}

void
Canvas::fill_circle(geom::fpoint const& center, float radius, surf::Color const& color)
{
  arc_points(center, radius, 0.0f, 360.0f, segments_for(radius, 360.0f), m_points);
  m_points.pop_back(); // last point duplicates the first
  add_polygon_fill(m_points, pack_color(color));
}

void
Canvas::draw_circle(geom::fpoint const& center, float radius, surf::Color const& color, float width)
{
  arc_points(center, radius, 0.0f, 360.0f, segments_for(radius, 360.0f), m_points);
  m_points.pop_back();
  add_line_strip(m_points, true, pack_color(color), width);
}

void
Canvas::fill_arc(geom::fpoint const& center, float radius, float start, float end, surf::Color const& color)
{
  arc_points(center, radius, start, end, segments_for(radius, end - start), m_points);
  m_points.insert(m_points.begin(), to_vec(center));
  add_polygon_fill(m_points, pack_color(color));
}

void
Canvas::draw_arc(geom::fpoint const& center, float radius, float start, float end,
                 surf::Color const& color, float width)
{
  arc_points(center, radius, start, end, segments_for(radius, end - start), m_points);
  add_line_strip(m_points, false, pack_color(color), width);
}

void
Canvas::fill_gradient(geom::frect const& rect,
                      surf::Color const& top_left, surf::Color const& top_right,
                      surf::Color const& bottom_right, surf::Color const& bottom_left)
{
  Vertex* v = add_quads({}, 1);
  v[0] = Vertex{rect.left(), rect.top(), 0.0f, 0.0f, pack_color(top_left)};
  v[1] = Vertex{rect.right(), rect.top(), 0.0f, 0.0f, pack_color(top_right)};
  v[2] = Vertex{rect.right(), rect.bottom(), 0.0f, 0.0f, pack_color(bottom_right)};
  v[3] = Vertex{rect.left(), rect.bottom(), 0.0f, 0.0f, pack_color(bottom_left)};
  transform_vertices(v, 4);
}

void
Canvas::fill_vertical_gradient(geom::frect const& rect, surf::Color const& top, surf::Color const& bottom)
{
  fill_gradient(rect, top, top, bottom, bottom);
}

void
Canvas::fill_horizontal_gradient(geom::frect const& rect, surf::Color const& left, surf::Color const& right)
{
  fill_gradient(rect, left, right, right, left);
}

/*} */

void
Canvas::draw_text(Font const& font, geom::fpoint const& pos, std::string_view text,
                  surf::Color const& color, TextAlign align)
{
  font.draw(*this, pos, text, color, align);
}

/*{ Raw geometry */

void
Canvas::draw_triangles(TextureId texture, std::span<Vertex const> vertices, std::span<uint16_t const> indices)
{
  if (vertices.empty() || indices.empty()) {
    return;
  }

  Vertex* v = add(texture, static_cast<uint32_t>(vertices.size()), indices);
  std::copy(vertices.begin(), vertices.end(), v);
  transform_vertices(v, vertices.size());
}

void
Canvas::draw_quads(TextureId texture, std::span<Vertex const> vertices)
{
  if (vertices.size() % 4 != 0) {
    throw std::invalid_argument("Canvas::draw_quads: vertex count must be a multiple of 4");
  }

  // split into chunks that fit into a single command
  constexpr size_t max_quads = max_command_vertices / 4;
  for (size_t offset = 0; offset < vertices.size(); offset += max_quads * 4) {
    size_t const count = std::min(vertices.size() - offset, max_quads * 4);
    Vertex* v = add_quads(texture, static_cast<uint32_t>(count / 4));
    std::copy_n(vertices.begin() + static_cast<std::ptrdiff_t>(offset), count, v);
    transform_vertices(v, count);
  }
}

void
Canvas::draw(StaticBatchId batch)
{
  add_object(CommandType::StaticBatch, ObjectDraw{.static_batch = batch, .mesh = {}, .matrix = glm::mat4(1.0f)});
}

void
Canvas::draw(MeshId mesh, glm::mat4 const& transform)
{
  add_object(CommandType::Mesh, ObjectDraw{.static_batch = {}, .mesh = mesh, .matrix = transform});
}

/*} */

} // namespace wstdisplay

/* EOF */
