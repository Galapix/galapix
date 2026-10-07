// SPDX-FileCopyrightText: 2002-2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <wstdisplay/compositor.hpp>

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include <glm/gtc/matrix_transform.hpp>

#include <wstdisplay/framebuffer.hpp>
#include <wstdisplay/renderer.hpp>
#include <wstdisplay/vertex.hpp>
#include <wstdisplay/view.hpp>

namespace wstdisplay {

std::vector<PassDesc>
Compositor::lighting_passes(surf::Color const& ambient)
{
  return {
    PassDesc{.name = "color"},
    PassDesc{.name = "light", .offscreen = true, .resolution = 0.25f,
             .clear_color = ambient, .composite_blend = Blend::Multiply},
    PassDesc{.name = "highlight"},
    PassDesc{.name = "overlay", .screen_space = true}
  };
}

Compositor::Compositor(std::vector<PassDesc> passes) :
  m_passes(),
  m_composite()
{
  m_composite.set_space(Space::Clip);
  for (auto& desc : passes) {
    m_passes.push_back(Pass{std::move(desc), std::make_unique<Canvas>(), {}});
  }
}

Compositor::~Compositor()
{
}

size_t
Compositor::get_pass_index(std::string_view name) const
{
  auto it = std::find_if(m_passes.begin(), m_passes.end(),
                         [name](Pass const& pass) { return pass.desc.name == name; });
  if (it == m_passes.end()) {
    throw std::out_of_range("Compositor: unknown pass: " + std::string(name));
  }
  return static_cast<size_t>(it - m_passes.begin());
}

PassDesc&
Compositor::get_pass(size_t index)
{
  return m_passes.at(index).desc;
}

PassDesc&
Compositor::get_pass(std::string_view name)
{
  return get_pass(get_pass_index(name));
}

Canvas&
Compositor::get_canvas(size_t index)
{
  return *m_passes.at(index).canvas;
}

Canvas&
Compositor::get_canvas(std::string_view name)
{
  return get_canvas(get_pass_index(name));
}

void
Compositor::set_enabled(std::string_view name, bool enabled)
{
  get_pass(name).enabled = enabled;
}

void
Compositor::render(Renderer& renderer, View const& view, FramebufferId target)
{
  Device& device = renderer.get_device();
  geom::isize const screen_size = target ? device.get(target).get_size() : renderer.get_frame_size();

  for (auto& pass : m_passes)
  {
    if (!pass.desc.enabled) {
      pass.canvas->clear();
      continue;
    }

    glm::mat4 const world_matrix = pass.desc.screen_space ? glm::mat4(1.0f) : view.get_matrix();

    if (!pass.desc.offscreen)
    {
      renderer.render(*pass.canvas, RenderPass{.target = target, .world_matrix = world_matrix});
    }
    else
    {
      float const res = pass.desc.resolution;
      geom::isize const size(std::max(1, static_cast<int>(std::ceil(static_cast<float>(screen_size.width()) * res))),
                             std::max(1, static_cast<int>(std::ceil(static_cast<float>(screen_size.height()) * res))));

      if (!pass.framebuffer || pass.framebuffer->get_size() != size) {
        pass.framebuffer = device.create_framebuffer(size);
      }

      // map screen pixels onto the smaller buffer
      glm::mat4 const scale = glm::scale(glm::mat4(1.0f),
                                         glm::vec3(static_cast<float>(size.width()) / static_cast<float>(screen_size.width()),
                                                   static_cast<float>(size.height()) / static_cast<float>(screen_size.height()),
                                                   1.0f));

      renderer.render(*pass.canvas, RenderPass{
          .target = pass.framebuffer,
          .clear = pass.desc.clear_color,
          .world_matrix = scale * world_matrix,
          .screen_matrix = scale
        });

      // clip space is y-up, row 0 of a framebuffer texture is the top
      Vertex const quad[] = {
        {-1.0f,  1.0f, 0.0f, 0.0f, packed_white},
        { 1.0f,  1.0f, 1.0f, 0.0f, packed_white},
        { 1.0f, -1.0f, 1.0f, 1.0f, packed_white},
        {-1.0f, -1.0f, 0.0f, 1.0f, packed_white},
      };
      m_composite.clear();
      m_composite.set_space(Space::Clip);
      m_composite.set_blend(pass.desc.composite_blend);
      m_composite.draw_quads(pass.framebuffer->get_texture(), quad);
      renderer.render(m_composite, RenderPass{.target = target});
    }

    pass.canvas->clear();
  }
}

void
Compositor::clear()
{
  for (auto& pass : m_passes) {
    pass.canvas->clear();
  }
}

} // namespace wstdisplay

/* EOF */
