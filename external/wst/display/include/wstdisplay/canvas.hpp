// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef HEADER_WSTDISPLAY_CANVAS_HPP
#define HEADER_WSTDISPLAY_CANVAS_HPP

#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

#include <glm/glm.hpp>
#include <geom/fwd.hpp>
#include <geom/point.hpp>
#include <geom/rect.hpp>
#include <surf/color.hpp>

#include "blend.hpp"
#include "draw_params.hpp"
#include "fwd.hpp"
#include "handle.hpp"
#include "vertex.hpp"

namespace wstdisplay {

/** The coordinate system vertices are given in */
enum class Space : uint8_t
{
  /** World coordinates, transformed by the world matrix of the
      RenderPass (e.g. a View) */
  World,

  /** Pixel coordinates of the render target, (0,0) is top left,
      unaffected by the View, for HUDs and overlays */
  Screen,

  /** Normalized device coordinates, [-1,1] covers the target, y
      points up. For full screen effects, see fill_screen(). */
  Clip
};

enum class TextAlign : uint8_t { Left, Center, Right };

enum class CommandType : uint8_t
{
  /** Vertices and indices stored in the canvas */
  Geometry,

  /** A StaticBatch, see Canvas::get_objects() */
  StaticBatch,

  /** A Mesh, see Canvas::get_objects() */
  Mesh
};

/** A clip rect together with the space it was specified in */
struct ClipRect
{
  geom::frect rect = {};
  Space space = Space::World;
};

/** A StaticBatch or Mesh draw */
struct ObjectDraw
{
  StaticBatchId static_batch = {};
  MeshId mesh = {};

  /** Model transform including the canvas transform at record time */
  glm::mat4 matrix = glm::mat4(1.0f);
};

/** Records drawing commands for later rendering with a Renderer.

    All draws are recorded as plain data: vertices go into one shared
    vertex array and each draw adds a small Command referencing them
    and the resources by handle. Before rendering the commands are
    stably sorted by z, so draws with equal z keep their submission
    order, and neighboring commands with the same texture, material,
    blend mode, clip rect and space are merged into a single GPU draw
    call. Putting surfaces on shared atlas pages is what makes
    batching effective, layers marked unordered batch even better.

    How draws are recorded is controlled by the canvas state: the
    transform, the clip rect, z, the blend mode, the material and the
    coordinate space. save() and restore() (or a Canvas::Scope)
    bracket temporary changes, like in the HTML canvas:

      Canvas::Scope scope(canvas);
      canvas.set_z(10.0f);
      canvas.set_blend(Blend::Add);
      canvas.translate(pos.x(), pos.y());
      canvas.draw(glow, geom::fpoint(0, 0));

    Resources are referenced by handle, a draw using a resource that
    has been destroyed and collected by the time the canvas is
    rendered is skipped. */
class Canvas final
{
public:
  struct Command
  {
    float z;
    uint32_t first_vertex;
    uint32_t vertex_count;
    uint32_t first_index;
    uint32_t index_count;

    /** Null draws untextured (with a white texture) */
    TextureId texture;

    /** Null uses the default program */
    MaterialId material;

    /** Index into get_objects() for StaticBatch and Mesh commands */
    uint32_t object;

    /** Index into get_clip_rects(), 0 is no clipping */
    uint16_t clip;

    Blend blend;
    Space space;
    CommandType type;
  };

  /** Maximum number of vertices a single command can hold, so that
      16-bit indices are enough (GLES 2.0) */
  static constexpr uint32_t max_command_vertices = 0xffff;

  /** Saves the canvas state on construction and restores it when
      going out of scope */
  class Scope final
  {
  public:
    explicit Scope(Canvas& canvas) : m_canvas(canvas) { m_canvas.save(); }
    ~Scope() { m_canvas.restore(); }

  private:
    Canvas& m_canvas;

  public:
    Scope(Scope const&) = delete;
    Scope& operator=(Scope const&) = delete;
  };

public:
  Canvas();

  /** Remove all commands and reset the state. The layer
      configuration (set_layer_unordered()) is kept. */
  void clear();

  bool empty() const { return m_commands.empty(); }

  /*{ State */
  /** Push the state: transform, clip rect, z, blend mode, material
      and space */
  void save();

  /** Pop the state pushed by the matching save() */
  void restore();

  /** Depth of the following draws, higher z is drawn on top. Draws
      with equal z are drawn in submission order. */
  void set_z(float z) { m_state.z = z; }
  float get_z() const { return m_state.z; }

  /** Blend mode of the following draws, Blend::Alpha by default */
  void set_blend(Blend blend) { m_state.blend = blend; }
  Blend get_blend() const { return m_state.blend; }

  /** Custom shader of the following draws, null for the default */
  void set_material(MaterialId material) { m_state.material = material; }
  MaterialId get_material() const { return m_state.material; }

  /** Coordinate space of the following draws, World by default */
  void set_space(Space space) { m_state.space = space; }
  Space get_space() const { return m_state.space; }
  /*} */

  /*{ Transform, applied to vertices when they are recorded */
  void translate(float x, float y);
  void scale(float x, float y);

  /** Rotate by \a degrees around the current origin, clockwise on
      screen */
  void rotate(float degrees);

  void set_transform(glm::mat3 const& transform);
  void mult_transform(glm::mat3 const& transform);
  void reset_transform();
  glm::mat3 const& get_transform() const { return m_state.transform; }
  /*} */

  /*{ Clipping */
  /** Clip the following draws to \a rect, given in current canvas
      coordinates and intersected with the current clip rect. A
      rotated rect clips to its bounding box. Undone by restore(). */
  void clip(geom::frect const& rect);
  /*} */

  /*{ Layers */
  /** Allow the renderer to reorder the draws with z equal to \a z
      for better batching, for layers where draws don't overlap or
      their order doesn't matter, e.g. particles, tiles or text.
      Within an unordered layer draws are grouped by material,
      texture and blend mode, draws with equal state keep their
      order. Kept across clear(). */
  void set_layer_unordered(float z, bool unordered = true);
  bool is_layer_unordered(float z) const;
  /*} */

  /*{ Surfaces */
  void draw(Surface const& surface, geom::fpoint const& pos);
  void draw(Surface const& surface, DrawParams const& params);

  /** Stretch the surface to \a dstrect, use Surface::region() to
      draw a part of it */
  void draw(Surface const& surface, geom::frect const& dstrect,
            surf::Color const& color = {1.0f, 1.0f, 1.0f});

  /** Fill \a rect by repeating \a pattern, \a offset shifts it. The
      surface must cover its whole texture, which needs
      TextureWrap::Repeat and, on GLES 2.0 without
      GL_OES_texture_npot, power-of-two dimensions. */
  void fill_pattern(Surface const& pattern, geom::frect const& rect, geom::foffset const& offset = {},
                    surf::Color const& color = {1.0f, 1.0f, 1.0f});
  /*} */

  /*{ Shapes, lines are drawn as quads of the given width, circles
      and arcs get enough segments to look round at their size */
  /** Cover the whole target, in Clip space, unaffected by the
      transform */
  void fill_screen(surf::Color const& color);

  void fill_rect(geom::frect const& rect, surf::Color const& color);
  void draw_rect(geom::frect const& rect, surf::Color const& color, float width = 1.0f);

  void fill_rounded_rect(geom::frect const& rect, float radius, surf::Color const& color);
  void draw_rounded_rect(geom::frect const& rect, float radius, surf::Color const& color, float width = 1.0f);

  void fill_quad(geom::fquad const& quad, surf::Color const& color);
  void draw_quad(geom::fquad const& quad, surf::Color const& color, float width = 1.0f);

  void fill_triangle(geom::fpoint const& p1, geom::fpoint const& p2, geom::fpoint const& p3,
                     surf::Color const& color);

  /** Fill a convex polygon */
  void fill_polygon(std::span<geom::fpoint const> points, surf::Color const& color);

  /** Draw the outline of a polygon, closing it */
  void draw_polygon(std::span<geom::fpoint const> points, surf::Color const& color, float width = 1.0f);

  void draw_line(geom::fpoint const& p1, geom::fpoint const& p2, surf::Color const& color, float width = 1.0f);

  /** Draw a connected, open line through \a points */
  void draw_polyline(std::span<geom::fpoint const> points, surf::Color const& color, float width = 1.0f);

  void fill_circle(geom::fpoint const& center, float radius, surf::Color const& color);
  void draw_circle(geom::fpoint const& center, float radius, surf::Color const& color, float width = 1.0f);

  /** Angles in degrees, clockwise starting at the positive x axis */
  void fill_arc(geom::fpoint const& center, float radius, float start, float end, surf::Color const& color);
  void draw_arc(geom::fpoint const& center, float radius, float start, float end,
                surf::Color const& color, float width = 1.0f);

  /** Fill with a color per corner */
  void fill_gradient(geom::frect const& rect,
                     surf::Color const& top_left, surf::Color const& top_right,
                     surf::Color const& bottom_right, surf::Color const& bottom_left);
  void fill_vertical_gradient(geom::frect const& rect, surf::Color const& top, surf::Color const& bottom);
  void fill_horizontal_gradient(geom::frect const& rect, surf::Color const& left, surf::Color const& right);
  /*} */

  /*{ Text, \a pos is the left/center/right end of the baseline of
      the first line, depending on \a align */
  void draw_text(Font const& font, geom::fpoint const& pos, std::string_view text,
                 surf::Color const& color = {1.0f, 1.0f, 1.0f}, TextAlign align = TextAlign::Left);
  /*} */

  /*{ Raw geometry, vertices are in current canvas coordinates */
  /** \a texture null draws untextured */
  void draw_triangles(TextureId texture, std::span<Vertex const> vertices, std::span<uint16_t const> indices);

  /** Four vertices per quad, in order top-left, top-right,
      bottom-right, bottom-left */
  void draw_quads(TextureId texture, std::span<Vertex const> vertices);

  /** Draw the prebuilt geometry of \a batch with the current
      transform */
  void draw(StaticBatchId batch);

  /** Draw \a mesh with \a transform applied on top of the current
      canvas transform */
  void draw(MeshId mesh, glm::mat4 const& transform);
  /*} */

  /*{ Recorded data */
  std::vector<Command> const& get_commands() const { return m_commands; }
  std::vector<Vertex> const& get_vertices() const { return m_vertices; }
  std::vector<uint16_t> const& get_indices() const { return m_indices; }

  /** Clip rects, entry 0 is unused */
  std::vector<ClipRect> const& get_clip_rects() const { return m_clip_rects; }

  /** StaticBatch and Mesh draws */
  std::vector<ObjectDraw> const& get_objects() const { return m_objects; }
  /*} */

private:
  struct State
  {
    glm::mat3 transform = glm::mat3(1.0f);
    bool identity = true;
    uint16_t clip = 0;
    float z = 0.0f;
    Blend blend = Blend::Alpha;
    Space space = Space::World;
    MaterialId material = {};
  };

  /** Append a command, reusing the previous one if all state matches.
      Returns the vertices to fill, indices are given relative to the
      first returned vertex. */
  Vertex* add(TextureId texture, uint32_t vertex_count, std::span<uint16_t const> indices);
  Vertex* add_quads(TextureId texture, uint32_t quad_count);
  void add_object(CommandType type, ObjectDraw const& object);

  /** One quad covering \a rect in canvas coordinates */
  void add_rect(TextureId texture, geom::frect const& rect, geom::frect const& uv, PackedColor color);

  glm::vec2 apply(float x, float y) const;
  void transform_vertices(Vertex* vertices, size_t count) const;

  /** Number of segments for an arc of \a radius covering \a degrees */
  int segments_for(float radius, float degrees) const;

  void add_polygon_fill(std::span<glm::vec2 const> points, PackedColor color);
  void add_line_strip(std::span<glm::vec2 const> points, bool loop, PackedColor color, float width);

private:
  std::vector<Command> m_commands;
  std::vector<Vertex> m_vertices;
  std::vector<uint16_t> m_indices;
  std::vector<ClipRect> m_clip_rects;
  std::vector<ObjectDraw> m_objects;

  State m_state;
  std::vector<State> m_state_stack;

  /** Sorted z values of the unordered layers */
  std::vector<float> m_unordered_layers;

  /** Scratch buffers for building shapes */
  std::vector<glm::vec2> m_points;
  std::vector<uint16_t> m_scratch_indices;

public:
  Canvas(Canvas const&) = delete;
  Canvas& operator=(Canvas const&) = delete;
};

} // namespace wstdisplay

#endif

/* EOF */
