// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

// Meshes and custom shaders: a lit, textured torus drawn as a Mesh
// with depth test, and the whole scene rendered into a Framebuffer
// that is then drawn with a shockwave distortion Material.
//
//   left mouse    start a shockwave at the pointer

#include <cmath>
#include <format>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>
#include <surf/palette.hpp>
#include <wstdisplay/framebuffer.hpp>
#include <wstdisplay/material.hpp>
#include <wstdisplay/mesh.hpp>
#include <wstdisplay/shader_program.hpp>
#include <wstdisplay/surface.hpp>
#include <wstdisplay/surface_manager.hpp>

#include "demo_app.hpp"

using namespace wstdisplay;

namespace {

char const lit_vert[] = R"(
attribute vec3 a_position;
attribute vec2 a_texcoord;
attribute vec4 a_color;
attribute vec3 a_normal;

uniform mat4 u_mvp;
uniform mat4 u_model;

varying vec2 v_texcoord;
varying vec4 v_color;
varying vec3 v_normal;

void main()
{
  v_texcoord = a_texcoord;
  v_color = a_color;
  // fine for uniform scaling
  v_normal = (u_model * vec4(a_normal, 0.0)).xyz;
  gl_Position = u_mvp * vec4(a_position, 1.0);
}
)";

char const lit_frag[] = R"(
uniform sampler2D u_texture0;
uniform vec3 u_light_dir;

varying vec2 v_texcoord;
varying vec4 v_color;
varying vec3 v_normal;

void main()
{
  vec3 n = normalize(v_normal);
  float diffuse = max(dot(n, normalize(u_light_dir)), 0.0);
  vec4 tex = texture2D(u_texture0, v_texcoord) * v_color;
  gl_FragColor = vec4(tex.rgb * (0.25 + 0.75 * diffuse), tex.a);
}
)";

char const shockwave_vert[] = R"(
attribute vec2 a_position;
attribute vec2 a_texcoord;
attribute vec4 a_color;

uniform mat4 u_mvp;

varying vec2 v_texcoord;
varying vec4 v_color;

void main()
{
  v_texcoord = a_texcoord;
  v_color = a_color;
  gl_Position = u_mvp * vec4(a_position, 0.0, 1.0);
}
)";

char const shockwave_frag[] = R"(
uniform sampler2D u_texture0;
uniform vec2 u_center;   // in texture coordinates
uniform float u_radius;  // in texture coordinates
uniform float u_aspect;

varying vec2 v_texcoord;
varying vec4 v_color;

void main()
{
  vec2 d = v_texcoord - u_center;
  d.x *= u_aspect;
  float dist = length(d);
  float ring = exp(-pow((dist - u_radius) * 25.0, 2.0));
  vec2 offset = (dist > 0.0) ? normalize(d) * ring * 0.03 : vec2(0.0);
  offset.x /= u_aspect;
  vec4 color = texture2D(u_texture0, v_texcoord - offset);
  gl_FragColor = vec4(color.rgb + vec3(ring * 0.15), 1.0) * v_color;
}
)";

surf::SoftwareSurface make_checker(int size)
{
  surf::SoftwareSurface image = surf::SoftwareSurface::create(surf::PixelFormat::RGBA8, geom::isize(size, size));
  for (int y = 0; y < size; ++y) {
    for (int x = 0; x < size; ++x) {
      bool const c = ((x / (size / 8)) + (y / (size / 8))) % 2 == 0;
      image.put_pixel(geom::ipoint(x, y), c ? surf::Color(1.0f, 0.85f, 0.3f) : surf::Color(0.8f, 0.2f, 0.2f));
    }
  }
  return image;
}

void make_torus(Mesh& mesh, float major, float minor, int rings, int sides)
{
  std::vector<MeshVertex> vertices;
  std::vector<uint16_t> indices;
  float const tau = 6.2831853f;

  for (int i = 0; i <= rings; ++i) {
    float const u = static_cast<float>(i) / static_cast<float>(rings);
    float const a = u * tau;
    for (int j = 0; j <= sides; ++j) {
      float const v = static_cast<float>(j) / static_cast<float>(sides);
      float const b = v * tau;
      glm::vec3 const normal(std::cos(a) * std::cos(b), std::sin(a) * std::cos(b), std::sin(b));
      glm::vec3 const pos = glm::vec3(std::cos(a) * major, std::sin(a) * major, 0.0f) + normal * minor;
      vertices.push_back(MeshVertex{pos.x, pos.y, pos.z, u * 4.0f, v, normal.x, normal.y, normal.z, packed_white});
    }
  }

  for (int i = 0; i < rings; ++i) {
    for (int j = 0; j < sides; ++j) {
      auto const idx = [&](int r, int s) { return static_cast<uint16_t>(r * (sides + 1) + s); };
      indices.insert(indices.end(), {idx(i, j), idx(i + 1, j), idx(i + 1, j + 1),
                                     idx(i, j), idx(i + 1, j + 1), idx(i, j + 1)});
    }
  }

  mesh.set_vertices(vertices);
  mesh.set_indices(indices);
}

} // namespace

class MeshDemo : public wstdemo::DemoApp
{
public:
  MeshDemo(int argc, char** argv) :
    DemoApp("Mesh", argc, argv),
    m_scene(),
    m_post(),
    m_surfaces(),
    m_tux(),
    m_checker(),
    m_torus(),
    m_lit_program(),
    m_shockwave_program(),
    m_lit(),
    m_shockwave(),
    m_framebuffer(),
    m_wave_center(0.5f, 0.5f),
    m_wave_time(0.0f)
  {}

  void init() override
  {
    Device& device = get_device();

    m_surfaces = std::make_unique<SurfaceManager>(device);
    m_tux = m_surfaces->get(get_data("tux.png"));

    m_lit_program = device.create_program(lit_vert, lit_frag);
    m_shockwave_program = device.create_program(shockwave_vert, shockwave_frag);
    m_lit = device.create_material(m_lit_program);
    m_shockwave = device.create_material(m_shockwave_program);

    m_checker = device.create_texture(make_checker(128), TextureParams{TextureFilter::Linear,
                                                                       TextureWrap::Repeat, TextureWrap::Repeat});
    m_torus = device.create_mesh();
    make_torus(*m_torus, 140.0f, 55.0f, 48, 24);
    m_torus->set_texture(m_checker);
    m_torus->set_material(m_lit);
    m_torus->set_depth_test(true);
    m_torus->set_blend(Blend::Opaque);
  }

  void on_event(SDL_Event const& event) override
  {
    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
      geom::fsize const window(get_window().get_size());
      m_wave_center = glm::vec2(static_cast<float>(event.button.x) / window.width(),
                                static_cast<float>(event.button.y) / window.height());
      m_wave_time = 0.0f;
    }
  }

  void update(float delta) override
  {
    m_wave_time += delta;
    if (m_wave_time > 2.5f) {
      // restart automatically, moving around the screen
      m_wave_time = 0.0f;
      float const t = get_time();
      m_wave_center = glm::vec2(0.5f + 0.3f * std::cos(t * 0.7f), 0.5f + 0.25f * std::sin(t * 1.1f));
    }
  }

  void draw(Renderer& renderer) override
  {
    float const t = get_time();
    geom::isize const size = get_size();
    geom::fsize const fsize(size);

    if (!m_framebuffer || m_framebuffer->get_size() != size) {
      m_framebuffer = get_device().create_framebuffer(size, FramebufferParams{.depth = true});
    }

    // 1. the scene into the framebuffer
    m_scene.clear();
    m_scene.fill_vertical_gradient(geom::frect(0, 0, fsize.width(), fsize.height()),
                                   surf::Color(0.1f, 0.15f, 0.3f), surf::Color(0.4f, 0.5f, 0.6f));
    for (float y = 40; y < fsize.height(); y += 160) {
      for (float x = 40; x < fsize.width(); x += 160) {
        m_scene.draw(m_tux, DrawParams().set_pos(geom::fpoint(x, y)).set_angle(std::sin(t + x * 0.01f) * 20.0f));
      }
    }

    glm::mat4 model(1.0f);
    model = glm::translate(model, glm::vec3(fsize.width() / 2.0f, fsize.height() / 2.0f, 0.0f));
    model = glm::rotate(model, t * 0.7f, glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, t * 0.4f, glm::vec3(0.0f, 1.0f, 0.0f));
    m_lit->set_uniform("u_light_dir", glm::vec3(0.4f, -0.6f, 0.7f));
    m_scene.set_z(1.0f);
    m_scene.draw(m_torus.get(), model);

    m_scene.set_z(2.0f);
    m_scene.draw_text(get_font(), geom::fpoint(fsize.width() / 2.0f, fsize.height() - 40.0f),
                      "Mesh with depth test, rendered into a Framebuffer, drawn with a shockwave Material",
                      surf::palette::white, TextAlign::Center);

    renderer.render(m_scene, RenderPass{.target = m_framebuffer, .clear = surf::palette::black});

    // 2. the framebuffer texture through the shockwave shader
    m_shockwave->set_uniform("u_center", m_wave_center);
    m_shockwave->set_uniform("u_radius", m_wave_time * 0.4f);
    m_shockwave->set_uniform("u_aspect", fsize.width() / fsize.height());

    m_post.clear();
    m_post.set_material(m_shockwave);
    m_post.set_blend(Blend::Opaque);
    m_post.draw(Surface(m_framebuffer->get_texture(), size), geom::fpoint(0, 0));
    renderer.render(m_post, RenderPass{.clear = surf::palette::black});
  }

private:
  Canvas m_scene;
  Canvas m_post;
  std::unique_ptr<SurfaceManager> m_surfaces;
  Surface m_tux;
  Unique<Texture> m_checker;
  Unique<Mesh> m_torus;
  Unique<ShaderProgram> m_lit_program;
  Unique<ShaderProgram> m_shockwave_program;
  Unique<Material> m_lit;
  Unique<Material> m_shockwave;
  Unique<Framebuffer> m_framebuffer;
  glm::vec2 m_wave_center;
  float m_wave_time;
};

WST_DEMO_MAIN(MeshDemo)

/* EOF */
