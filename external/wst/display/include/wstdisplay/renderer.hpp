// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef HEADER_WSTDISPLAY_RENDERER_HPP
#define HEADER_WSTDISPLAY_RENDERER_HPP

#include <memory>
#include <optional>

#include <glm/glm.hpp>
#include <geom/rect.hpp>
#include <geom/size.hpp>
#include <surf/color.hpp>
#include <surf/software_surface.hpp>

#include "caps.hpp"
#include "fwd.hpp"
#include "handle.hpp"

namespace wstdisplay {

struct RenderStats
{
  int canvases = 0;
  int commands = 0;
  int batches = 0;
  int draw_calls = 0;
  int vertices = 0;
  int triangles = 0;
  int texture_binds = 0;
  int program_binds = 0;
  int target_switches = 0;

  /** Draws skipped because a resource they use has been destroyed */
  int skipped = 0;
};

/** Where and how a Canvas is rendered */
struct RenderPass
{
  /** The framebuffer to render into, null for the frame's target
      (the window, or a toolkit provided framebuffer) */
  FramebufferId target = {};

  /** Clear the target to this color first, depth and stencil are
      cleared as well */
  std::optional<surf::Color> clear = std::nullopt;

  /** Maps Space::World to target pixels, e.g. View::get_matrix() */
  glm::mat4 world_matrix = glm::mat4(1.0f);

  /** Maps Space::Screen to target pixels, identity unless rendering
      into a target of a different size than the screen */
  glm::mat4 screen_matrix = glm::mat4(1.0f);
};

/** Executes Canvas contents on the OpenGL context of a Device.

    The Renderer works on whatever framebuffer is bound when the frame
    begins, so it can be used with OpenGLWindow as well as with a
    context owned by a toolkit (e.g. a Gtk::GLArea). It keeps a cache
    of the GL state it touches.

    A frame looks like:

      renderer.begin_frame(window_size);
      renderer.render(canvas, {.clear = black, .world_matrix = view.get_matrix()});
      renderer.render(hud);
      renderer.end_frame();
      swap buffers */
class Renderer final
{
public:
  explicit Renderer(Device& device);
  ~Renderer();

  Device& get_device() const;
  Caps const& get_caps() const;

  /** Start a frame on the framebuffer that is currently bound with
      the given size. Resets the state cache and the stats. */
  void begin_frame(geom::isize const& size);

  /** Finish the frame, restore the GL state and delete the resources
      destroyed during the frame (Device::collect()) */
  void end_frame();

  geom::isize get_frame_size() const;

  /** Clear \a target, null for the frame's target */
  void clear(surf::Color const& color, FramebufferId target = {});

  void render(Canvas const& canvas, RenderPass const& pass = {});

  /** Render to the frame's target with World space mapped through \a view */
  void render(Canvas const& canvas, View const& view);

  /** Read back the pixels of \a target as RGBA8, top row first */
  surf::SoftwareSurface read_pixels(FramebufferId target = {}) const;
  surf::SoftwareSurface read_pixels(geom::irect const& rect, FramebufferId target = {}) const;

  RenderStats const& get_stats() const;

private:
  class Impl;
  std::unique_ptr<Impl> m_impl;

public:
  Renderer(Renderer const&) = delete;
  Renderer& operator=(Renderer const&) = delete;
};

} // namespace wstdisplay

#endif

/* EOF */
