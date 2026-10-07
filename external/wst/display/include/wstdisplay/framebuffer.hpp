// SPDX-FileCopyrightText: 2002-2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef HEADER_WSTDISPLAY_FRAMEBUFFER_HPP
#define HEADER_WSTDISPLAY_FRAMEBUFFER_HPP

#include <cstdint>

#include <geom/size.hpp>

#include "device.hpp"
#include "fwd.hpp"

namespace wstdisplay {

/** An offscreen render target backed by an RGBA texture, owned by a
    Device and created with Device::create_framebuffer(). Rendered
    content is stored with the top row first like any uploaded image,
    so the texture can be drawn like any other. */
class Framebuffer final
{
public:
  Framebuffer(Device& device, geom::isize const& size, FramebufferParams const& params);
  ~Framebuffer();

  geom::isize get_size() const { return m_size; }
  int get_width() const { return m_size.width(); }
  int get_height() const { return m_size.height(); }

  /** The color buffer, owned by the framebuffer */
  TextureId get_texture() const { return m_texture; }

  bool has_depth() const { return m_depth_buffer != 0; }

  /** The OpenGL framebuffer name */
  uint32_t get_handle() const { return m_handle; }

private:
  void attach_renderbuffer(uint32_t& handle, uint32_t format, uint32_t attachment);
  void destroy();

private:
  geom::isize m_size;
  Unique<Texture> m_texture;
  uint32_t m_handle;
  uint32_t m_depth_buffer;
  uint32_t m_stencil_buffer;

public:
  Framebuffer(Framebuffer const&) = delete;
  Framebuffer& operator=(Framebuffer const&) = delete;
};

} // namespace wstdisplay

#endif

/* EOF */
