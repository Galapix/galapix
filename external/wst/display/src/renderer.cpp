// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <wstdisplay/renderer.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <stdexcept>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <logmich/log.hpp>
#include <surf/palette.hpp>

#include <wstdisplay/batch.hpp>
#include <wstdisplay/canvas.hpp>
#include "device_impl.hpp"
#include "gl.hpp"
#include <wstdisplay/view.hpp>

namespace wstdisplay {

namespace {

char const default_vert_source[] = R"(
attribute vec2 a_position;
attribute vec2 a_texcoord;
attribute vec4 a_color;
attribute vec4 a_effect;

uniform mat4 u_mvp;

varying vec2 v_texcoord;
varying vec4 v_color;
varying vec4 v_effect;

void main()
{
  v_texcoord = a_texcoord;
  v_color = a_color;
  v_effect = a_effect;
  gl_Position = u_mvp * vec4(a_position, 0.0, 1.0);
}
)";

char const mesh_vert_source[] = R"(
attribute vec3 a_position;
attribute vec2 a_texcoord;
attribute vec4 a_color;

uniform mat4 u_mvp;

varying vec2 v_texcoord;
varying vec4 v_color;
varying vec4 v_effect;

void main()
{
  v_texcoord = a_texcoord;
  v_color = a_color;
  v_effect = vec4(0.0);
  gl_Position = u_mvp * vec4(a_position, 1.0);
}
)";

char const default_frag_source[] = R"(
uniform sampler2D u_texture0;

varying vec2 v_texcoord;
varying vec4 v_color;
varying vec4 v_effect;

void main()
{
  vec4 color = texture2D(u_texture0, v_texcoord) * v_color;
  gl_FragColor = vec4(mix(color.rgb, v_effect.rgb, v_effect.a), color.a);
}
)";

constexpr GLuint invalid_handle = 0xffffffffu;

/** Meshes can extend this far in front of and behind the screen, in pixels */
constexpr float depth_range = 4096.0f;

} // namespace

class Renderer::Impl
{
public:
  Device& device;
  Device::Impl& resources;

  Unique<ShaderProgram> default_program;
  Unique<ShaderProgram> mesh_program;
  Unique<Texture> white_texture;

  GLuint vao = 0;
  GLuint vertex_buffer = 0;
  GLuint index_buffer = 0;

  BatchList batches;

  RenderStats stats;
  bool warned_skipped = false;

  GLint frame_framebuffer = 0;
  geom::isize frame_size = geom::isize(1, 1);

  /** The target of the current pass, nullptr for the frame's target */
  Framebuffer const* target = nullptr;
  bool target_bound = false;

  /*{ state cache, invalidated at the start of every render() */
  GLuint cur_program = invalid_handle;
  std::array<GLuint, ShaderProgram::max_textures> cur_textures = {};
  int cur_active_unit = -1;
  int cur_blend = -1;
  int cur_scissor_enabled = -1;
  geom::irect cur_scissor = {};
  Material const* cur_material = nullptr;
  bool cur_material_valid = false;
  glm::mat4 cur_mvp = glm::mat4(0.0f);
  bool cur_mvp_valid = false;
  int64_t cur_attrib_base = -1;
  /*} */

public:
  Impl(Device& device_, Device::Impl& resources_) :
    device(device_),
    resources(resources_),
    default_program(),
    mesh_program(),
    white_texture(),
    batches(),
    stats()
  {
    if (!device.get_caps().is_gles()) {
      // a core profile requires a VAO to be bound for drawing
      glGenVertexArrays(1, &vao);
    }
    glGenBuffers(1, &vertex_buffer);
    glGenBuffers(1, &index_buffer);

    default_program = device.create_program(default_vert_source, default_frag_source);
    mesh_program = device.create_program(mesh_vert_source, default_frag_source);
    white_texture = device.create_texture(
      surf::SoftwareSurface::create(surf::PixelFormat::RGBA8, geom::isize(1, 1), surf::palette::white));

    WST_CHECK_GL("initializing renderer");
  }

  Impl(Impl const&) = delete;
  Impl& operator=(Impl const&) = delete;

  ~Impl()
  {
    glDeleteBuffers(1, &vertex_buffer);
    glDeleteBuffers(1, &index_buffer);
    if (vao) {
      glDeleteVertexArrays(1, &vao);
    }
  }

  geom::isize target_size() const
  {
    return target ? target->get_size() : frame_size;
  }

  /** Target pixel coordinates to clip space. Framebuffers are
      rendered upside down so that row 0 of their texture is the top
      row, like any uploaded image. */
  glm::mat4 projection() const
  {
    geom::isize const size = target_size();
    float const w = static_cast<float>(size.width());
    float const h = static_cast<float>(size.height());
    // the depth range leaves room for meshes, 2D geometry is at z = 0
    if (target) {
      return glm::ortho(0.0f, w, 0.0f, h, -depth_range, depth_range);
    } else {
      return glm::ortho(0.0f, w, h, 0.0f, -depth_range, depth_range);
    }
  }

  void bind_target(FramebufferId framebuffer)
  {
    Framebuffer const* const fb = framebuffer ? &device.get(framebuffer) : nullptr;
    if (target_bound && fb == target) {
      return;
    }

    target = fb;
    target_bound = true;

    geom::isize const size = target_size();
    glBindFramebuffer(GL_FRAMEBUFFER, target ? target->get_handle() : static_cast<GLuint>(frame_framebuffer));
    glViewport(0, 0, size.width(), size.height());
    stats.target_switches += 1;
  }

  void clear(surf::Color const& color)
  {
    glDisable(GL_SCISSOR_TEST);
    cur_scissor_enabled = 0;
    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  }

  void skip()
  {
    stats.skipped += 1;
    if (!warned_skipped) {
      log_warn("Renderer: skipping draws that use destroyed resources");
      warned_skipped = true;
    }
  }

  void invalidate_cache()
  {
    cur_program = invalid_handle;
    cur_textures.fill(invalid_handle);
    cur_active_unit = -1;
    cur_blend = -1;
    cur_scissor_enabled = -1;
    cur_material_valid = false;
    cur_mvp_valid = false;
    cur_attrib_base = -1;
  }

  void use_program(ShaderProgram const& program)
  {
    if (cur_program != program.get_handle()) {
      glUseProgram(program.get_handle());
      cur_program = program.get_handle();
      cur_mvp_valid = false;
      cur_material_valid = false;
      stats.program_binds += 1;
    }
  }

  void set_mvp(ShaderProgram const& program, glm::mat4 const& mvp)
  {
    if (!cur_mvp_valid || cur_mvp != mvp) {
      if (program.get_mvp_location() != -1) {
        glUniformMatrix4fv(program.get_mvp_location(), 1, GL_FALSE, glm::value_ptr(mvp));
      }
      cur_mvp = mvp;
      cur_mvp_valid = true;
    }
  }

  void bind_texture(int unit, GLuint handle)
  {
    if (cur_textures[static_cast<size_t>(unit)] != handle) {
      if (cur_active_unit != unit) {
        glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(unit));
        cur_active_unit = unit;
      }
      glBindTexture(GL_TEXTURE_2D, handle);
      cur_textures[static_cast<size_t>(unit)] = handle;
      stats.texture_binds += 1;
    }
  }

  void set_blend(Blend blend)
  {
    int const b = static_cast<int>(blend);
    if (cur_blend == b) {
      return;
    }

    if (blend == Blend::Opaque) {
      glDisable(GL_BLEND);
    } else {
      if (cur_blend == -1 || cur_blend == static_cast<int>(Blend::Opaque)) {
        glEnable(GL_BLEND);
      }

      switch (blend)
      {
        case Blend::Alpha:
          glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
          break;

        case Blend::Premultiplied:
          glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
          break;

        case Blend::Add:
          glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_ZERO, GL_ONE);
          break;

        case Blend::Multiply:
          glBlendFuncSeparate(GL_DST_COLOR, GL_ZERO, GL_ZERO, GL_ONE);
          break;

        case Blend::Opaque:
          break;
      }
    }
    cur_blend = b;
  }

  void set_scissor(std::optional<geom::irect> const& rect)
  {
    if (!rect) {
      if (cur_scissor_enabled != 0) {
        glDisable(GL_SCISSOR_TEST);
        cur_scissor_enabled = 0;
      }
    } else {
      if (cur_scissor_enabled != 1) {
        glEnable(GL_SCISSOR_TEST);
        cur_scissor_enabled = 1;
      }
      if (cur_scissor != *rect) {
        glScissor(rect->left(), rect->top(), rect->width(), rect->height());
        cur_scissor = *rect;
      }
    }
  }

  /** The texture to bind for \a handle, white for null, nullptr if stale */
  Texture const* resolve_texture(TextureId handle) const
  {
    return handle ? resources.textures.find(handle) : resources.textures.find(white_texture.get());
  }

  /** Program and material for a draw, false if a resource is stale */
  bool resolve_material(MaterialId handle, ShaderProgram const& fallback,
                        ShaderProgram const*& program, Material const*& material) const
  {
    if (!handle) {
      program = &fallback;
      material = nullptr;
      return true;
    }

    material = resources.materials.find(handle);
    program = material ? resources.programs.find(material->get_program()) : nullptr;
    return program != nullptr;
  }

  void apply_material(Material const* material)
  {
    if (cur_material_valid && cur_material == material) {
      return;
    }

    if (material) {
      for (int unit = 1; unit < ShaderProgram::max_textures; ++unit) {
        TextureId const handle = material->get_texture(unit);
        if (handle) {
          Texture const* texture = resources.textures.find(handle);
          bind_texture(unit, texture ? texture->get_handle() : 0);
        }
      }

      for (auto const& uniform : material->get_uniforms()) {
        float const* d = uniform.data.data();
        switch (uniform.type)
        {
          case Material::UniformType::Int: glUniform1i(uniform.location, static_cast<GLint>(d[0])); break;
          case Material::UniformType::Float: glUniform1f(uniform.location, d[0]); break;
          case Material::UniformType::Vec2: glUniform2fv(uniform.location, 1, d); break;
          case Material::UniformType::Vec3: glUniform3fv(uniform.location, 1, d); break;
          case Material::UniformType::Vec4: glUniform4fv(uniform.location, 1, d); break;
          case Material::UniformType::Mat4: glUniformMatrix4fv(uniform.location, 1, GL_FALSE, d); break;
        }
      }
    }

    cur_material = material;
    cur_material_valid = true;
  }

  void set_vertex_layout(int64_t base_vertex)
  {
    if (cur_attrib_base == base_vertex) {
      return;
    }

    auto const offset = [base_vertex](size_t member) {
      return reinterpret_cast<void const*>(static_cast<uintptr_t>(base_vertex) * sizeof(Vertex) + member);
    };

    glVertexAttribPointer(static_cast<GLuint>(Attrib::Position), 2, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex), offset(offsetof(Vertex, x)));
    glVertexAttribPointer(static_cast<GLuint>(Attrib::Texcoord), 2, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex), offset(offsetof(Vertex, u)));
    glVertexAttribPointer(static_cast<GLuint>(Attrib::Color), 4, GL_UNSIGNED_BYTE, GL_TRUE,
                          sizeof(Vertex), offset(offsetof(Vertex, color)));
    glVertexAttribPointer(static_cast<GLuint>(Attrib::Effect), 4, GL_UNSIGNED_BYTE, GL_TRUE,
                          sizeof(Vertex), offset(offsetof(Vertex, effect)));
    cur_attrib_base = base_vertex;
  }

  void enable_2d_attribs()
  {
    glEnableVertexAttribArray(static_cast<GLuint>(Attrib::Position));
    glEnableVertexAttribArray(static_cast<GLuint>(Attrib::Texcoord));
    glEnableVertexAttribArray(static_cast<GLuint>(Attrib::Color));
    glEnableVertexAttribArray(static_cast<GLuint>(Attrib::Effect));
    glDisableVertexAttribArray(static_cast<GLuint>(Attrib::Normal));
  }

  /** Convert a clip rect to a scissor rect in GL window coordinates */
  std::optional<geom::irect> scissor_for(ClipRect const& clip, glm::mat4 const& mvp) const
  {
    geom::isize const size = target_size();
    geom::frect const& r = clip.rect;
    glm::vec4 const corners[] = {
      mvp * glm::vec4(r.left(), r.top(), 0.0f, 1.0f),
      mvp * glm::vec4(r.right(), r.top(), 0.0f, 1.0f),
      mvp * glm::vec4(r.right(), r.bottom(), 0.0f, 1.0f),
      mvp * glm::vec4(r.left(), r.bottom(), 0.0f, 1.0f)
    };

    float x0 = corners[0].x;
    float x1 = corners[0].x;
    float y0 = corners[0].y;
    float y1 = corners[0].y;
    for (auto const& c : corners) {
      x0 = std::min(x0, c.x);
      x1 = std::max(x1, c.x);
      y0 = std::min(y0, c.y);
      y1 = std::max(y1, c.y);
    }

    float const w = static_cast<float>(size.width());
    float const h = static_cast<float>(size.height());
    int const left = static_cast<int>(std::round((x0 + 1.0f) / 2.0f * w));
    int const right = static_cast<int>(std::round((x1 + 1.0f) / 2.0f * w));
    int const bottom = static_cast<int>(std::round((y0 + 1.0f) / 2.0f * h));
    int const top = static_cast<int>(std::round((y1 + 1.0f) / 2.0f * h));

    // irect(left, top, right, bottom) used as (x, y, x+w, y+h) in GL window coordinates
    return geom::irect(left, bottom, std::max(left, right), std::max(bottom, top));
  }

  /** Draw a run of indices from the currently bound buffers */
  void draw_geometry(Batch const& batch, glm::mat4 const& mvp)
  {
    Texture const* const texture = resolve_texture(batch.texture);
    ShaderProgram const* program = nullptr;
    Material const* material = nullptr;
    if (!texture || !resolve_material(batch.material, *default_program, program, material)) {
      skip();
      return;
    }

    use_program(*program);
    set_mvp(*program, mvp);
    apply_material(material);
    bind_texture(0, texture->get_handle());
    set_blend(batch.blend);
    set_vertex_layout(batch.base_vertex);

    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(batch.index_count), GL_UNSIGNED_SHORT,
                   reinterpret_cast<void const*>(static_cast<uintptr_t>(batch.first_index) * sizeof(uint16_t)));

    stats.batches += 1;
    stats.draw_calls += 1;
    stats.triangles += static_cast<int>(batch.index_count / 3);
  }

  void bind_canvas_buffers()
  {
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
    enable_2d_attribs();
    cur_attrib_base = -1;
  }

  void render(Canvas const& canvas, RenderPass const& pass)
  {
    bind_target(pass.target);
    if (pass.clear) {
      clear(*pass.clear);
    }

    if (canvas.empty()) {
      return;
    }

    build_batches(canvas, batches);

    stats.canvases += 1;
    stats.commands += static_cast<int>(canvas.get_commands().size());

    invalidate_cache();

    if (vao) {
      glBindVertexArray(vao);
    }

    bind_canvas_buffers();
    if (!batches.vertices.empty()) {
      glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(batches.vertices.size() * sizeof(Vertex)),
                   batches.vertices.data(), GL_STREAM_DRAW);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(batches.indices.size() * sizeof(uint16_t)),
                   batches.indices.data(), GL_STREAM_DRAW);
    }

    glm::mat4 const proj = projection();
    std::array<glm::mat4, 3> const space_mvp = {
      proj * pass.world_matrix,  // World
      proj * pass.screen_matrix, // Screen
      glm::mat4(1.0f)            // Clip
    };

    auto const& clips = canvas.get_clip_rects();
    auto const& objects = canvas.get_objects();

    for (Batch const& batch : batches.batches)
    {
      glm::mat4 const& mvp = space_mvp[static_cast<size_t>(batch.space)];

      set_scissor(batch.clip == 0 ? std::nullopt :
                  scissor_for(clips[batch.clip], space_mvp[static_cast<size_t>(clips[batch.clip].space)]));

      switch (batch.type)
      {
        case CommandType::Geometry:
          draw_geometry(batch, mvp);
          break;

        case CommandType::StaticBatch:
          draw_static_batch(objects[batch.object].static_batch, mvp * objects[batch.object].matrix);
          break;

        case CommandType::Mesh:
          draw_mesh(objects[batch.object].mesh, mvp, objects[batch.object].matrix);
          break;
      }
    }

    stats.vertices += static_cast<int>(batches.vertices.size());

    set_scissor(std::nullopt);
    WST_CHECK_GL("rendering canvas");
  }

  void draw_static_batch(StaticBatchId handle, glm::mat4 const& mvp)
  {
    StaticBatch const* const sbatch = resources.static_batches.find(handle);
    if (!sbatch) {
      skip();
      return;
    }

    sbatch->upload();

    glBindBuffer(GL_ARRAY_BUFFER, sbatch->get_vertex_buffer());
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sbatch->get_index_buffer());
    cur_attrib_base = -1;

    for (Batch const& batch : sbatch->get_batches().batches) {
      if (batch.type == CommandType::Geometry) {
        draw_geometry(batch, mvp);
      }
    }

    bind_canvas_buffers();
  }

  void draw_mesh(MeshId handle, glm::mat4 const& mvp, glm::mat4 const& model)
  {
    Mesh const* const mesh = resources.meshes.find(handle);
    Texture const* const texture = mesh ? resolve_texture(mesh->get_texture()) : nullptr;
    ShaderProgram const* program = nullptr;
    Material const* material = nullptr;
    if (!mesh || !texture || !resolve_material(mesh->get_material(), *mesh_program, program, material)) {
      skip();
      return;
    }

    mesh->upload();

    use_program(*program);
    set_mvp(*program, mvp * model);
    apply_material(material);
    bind_texture(0, texture->get_handle());
    set_blend(mesh->get_blend());

    int const model_location = program->get_uniform_location("u_model");
    if (model_location != -1) {
      glUniformMatrix4fv(model_location, 1, GL_FALSE, glm::value_ptr(model));
    }

    // meshes have no effect values
    glDisableVertexAttribArray(static_cast<GLuint>(Attrib::Effect));
    glVertexAttrib4f(static_cast<GLuint>(Attrib::Effect), 0.0f, 0.0f, 0.0f, 0.0f);

    if (mesh->get_depth_test()) {
      glEnable(GL_DEPTH_TEST);
      glDepthFunc(GL_LEQUAL);
    }

    mesh->draw();

    if (mesh->get_depth_test()) {
      glDisable(GL_DEPTH_TEST);
    }

    stats.draw_calls += 1;
    stats.triangles += mesh->get_triangle_count();

    // meshes use their own buffers, restore the canvas ones
    bind_canvas_buffers();
  }
};

Renderer::Renderer(Device& device) :
  m_impl(std::make_unique<Impl>(device, *device.m_impl))
{
}

Renderer::~Renderer()
{
}

Device&
Renderer::get_device() const
{
  return m_impl->device;
}

Caps const&
Renderer::get_caps() const
{
  return m_impl->device.get_caps();
}

void
Renderer::begin_frame(geom::isize const& size)
{
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &m_impl->frame_framebuffer);
  m_impl->frame_size = size;
  m_impl->target = nullptr;
  m_impl->target_bound = false;
  m_impl->stats = RenderStats();
  m_impl->invalidate_cache();

  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
  glDisable(GL_SCISSOR_TEST);

  m_impl->bind_target({});
}

void
Renderer::end_frame()
{
  m_impl->bind_target({});

  glUseProgram(0);
  glDisable(GL_SCISSOR_TEST);
  glDisable(GL_BLEND);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, 0);
  if (m_impl->vao) {
    glBindVertexArray(0);
  }
  m_impl->invalidate_cache();
  WST_CHECK_GL("end of frame");

  m_impl->device.collect();
}

geom::isize
Renderer::get_frame_size() const
{
  return m_impl->frame_size;
}

void
Renderer::clear(surf::Color const& color, FramebufferId target)
{
  m_impl->bind_target(target);
  m_impl->clear(color);
}

void
Renderer::render(Canvas const& canvas, RenderPass const& pass)
{
  m_impl->render(canvas, pass);
}

void
Renderer::render(Canvas const& canvas, View const& view)
{
  m_impl->render(canvas, RenderPass{.world_matrix = view.get_matrix()});
}

surf::SoftwareSurface
Renderer::read_pixels(FramebufferId target) const
{
  geom::isize const size = target ? m_impl->device.get(target).get_size() : m_impl->frame_size;
  return read_pixels(geom::irect(size), target);
}

surf::SoftwareSurface
Renderer::read_pixels(geom::irect const& rect, FramebufferId target) const
{
  m_impl->bind_target(target);

  geom::isize const size = m_impl->target_size();
  std::vector<uint8_t> pixels(static_cast<size_t>(rect.width()) * static_cast<size_t>(rect.height()) * 4);

  // the default framebuffer has row 0 at the bottom, framebuffers at the top
  bool const flipped = (m_impl->target == nullptr);
  int const y = flipped ? size.height() - rect.bottom() : rect.top();

  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(rect.left(), y, rect.width(), rect.height(), GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
  WST_CHECK_GL("reading pixels");

  surf::SoftwareSurface surface = surf::SoftwareSurface::create(surf::PixelFormat::RGBA8, rect.size());
  size_t const row_size = static_cast<size_t>(rect.width()) * 4;
  for (int row = 0; row < rect.height(); ++row) {
    int const src_row = flipped ? rect.height() - 1 - row : row;
    std::memcpy(surface.get_row_data(row), pixels.data() + row_size * static_cast<size_t>(src_row), row_size);
  }
  return surface;
}

RenderStats const&
Renderer::get_stats() const
{
  return m_impl->stats;
}

} // namespace wstdisplay

/* EOF */
