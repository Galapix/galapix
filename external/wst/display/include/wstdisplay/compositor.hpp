// SPDX-FileCopyrightText: 2002-2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef HEADER_WSTDISPLAY_COMPOSITOR_HPP
#define HEADER_WSTDISPLAY_COMPOSITOR_HPP

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <surf/color.hpp>

#include "blend.hpp"
#include "canvas.hpp"
#include "device.hpp"
#include "fwd.hpp"

namespace wstdisplay {

/** Description of a render pass of the Compositor */
struct PassDesc
{
  std::string name = {};

  /** Render into an offscreen buffer that is then combined with the
      screen using composite_blend, e.g. a lightmap. Otherwise the
      pass renders directly onto the screen. */
  bool offscreen = false;

  /** Size of the offscreen buffer relative to the screen */
  float resolution = 1.0f;

  /** The offscreen buffer is cleared to this color every frame, for
      a lightmap this is the ambient light */
  surf::Color clear_color = {0.0f, 0.0f, 0.0f, 0.0f};

  Blend composite_blend = Blend::Alpha;

  /** World space of the pass is screen pixels, the View is ignored.
      For HUDs and editor overlays. */
  bool screen_space = false;

  bool enabled = true;
};

/** Renders a scene made up of several passes, each with a Canvas of
    its own, in order. The passes are configurable, lighting_passes()
    gives the classic color/light/highlight/overlay setup.

    Typical use:

      compositor.get_canvas("color").draw(...);
      compositor.get_canvas("light").draw(light, pos);
      compositor.render(renderer, view);  // clears the canvases

    Offscreen buffers are created on the renderer's Device as needed. */
class Compositor final
{
public:
  /** color: regular graphics
      light: offscreen at 1/4 resolution, cleared to \a ambient and
             multiplied with the screen, draw lights with Blend::Add
      highlight: glows on top of the lighting, use Blend::Add
      overlay: screen space, for HUD and controls */
  static std::vector<PassDesc> lighting_passes(surf::Color const& ambient = {0.0f, 0.0f, 0.0f});

public:
  explicit Compositor(std::vector<PassDesc> passes);
  ~Compositor();

  size_t get_pass_count() const { return m_passes.size(); }

  /** Index of the pass \a name, throws if unknown */
  size_t get_pass_index(std::string_view name) const;

  PassDesc& get_pass(size_t index);
  PassDesc& get_pass(std::string_view name);

  Canvas& get_canvas(size_t index);
  Canvas& get_canvas(std::string_view name);

  void set_enabled(std::string_view name, bool enabled);

  /** Render all enabled passes onto \a target, null for the frame's
      target, and clear all canvases */
  void render(Renderer& renderer, View const& view, FramebufferId target = {});

  /** Clear all canvases without rendering */
  void clear();

private:
  struct Pass
  {
    PassDesc desc;
    std::unique_ptr<Canvas> canvas;
    Unique<Framebuffer> framebuffer;
  };

  std::vector<Pass> m_passes;

  /** Draws the offscreen buffers onto the target */
  Canvas m_composite;

public:
  Compositor(Compositor const&) = delete;
  Compositor& operator=(Compositor const&) = delete;
};

} // namespace wstdisplay

#endif

/* EOF */
