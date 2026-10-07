// SPDX-FileCopyrightText: 2002-2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <wstdisplay/framebuffer.hpp>

#include <stdexcept>
#include <string>

#include "gl.hpp"
#include <wstdisplay/texture.hpp>

namespace wstdisplay {

namespace {

char const* framebuffer_status_string(GLenum status)
{
  switch (status)
  {
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: return "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: return "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
    case GL_FRAMEBUFFER_UNSUPPORTED: return "GL_FRAMEBUFFER_UNSUPPORTED";
    default: return "unknown framebuffer status";
  }
}

} // namespace

void
Framebuffer::attach_renderbuffer(uint32_t& handle, uint32_t format, uint32_t attachment)
{
  glGenRenderbuffers(1, &handle);
  glBindRenderbuffer(GL_RENDERBUFFER, handle);
  glRenderbufferStorage(GL_RENDERBUFFER, format, m_size.width(), m_size.height());
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, handle);
  glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

Framebuffer::Framebuffer(Device& device, geom::isize const& size, FramebufferParams const& params) :
  m_size(size),
  m_texture(device.create_texture(size, params.texture)),
  m_handle(0),
  m_depth_buffer(0),
  m_stencil_buffer(0)
{
  bool const depth = params.depth;
  bool const stencil = params.stencil;

  GLint old_framebuffer = 0;
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &old_framebuffer);

  glGenFramebuffers(1, &m_handle);
  glBindFramebuffer(GL_FRAMEBUFFER, m_handle);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         m_texture->get_handle(), 0);

  bool const gles = device.get_caps().is_gles();
  constexpr GLenum depth24_stencil8 = 0x88F0; // GL_DEPTH24_STENCIL8 and GL_DEPTH24_STENCIL8_OES

  if (depth && stencil && !gles) {
    attach_renderbuffer(m_depth_buffer, depth24_stencil8, GL_DEPTH_STENCIL_ATTACHMENT);
  } else {
    // GLES 2.0 only guarantees 16 bit depth and separate 8 bit stencil
    if (depth) {
      attach_renderbuffer(m_depth_buffer, gles ? GL_DEPTH_COMPONENT16 : GL_DEPTH_COMPONENT24,
                          GL_DEPTH_ATTACHMENT);
    }
    if (stencil) {
      attach_renderbuffer(m_stencil_buffer, GL_STENCIL_INDEX8, GL_STENCIL_ATTACHMENT);
    }
  }

  GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

  if (status != GL_FRAMEBUFFER_COMPLETE && gles && depth && stencil) {
    // some GLES drivers only support depth and stencil packed together
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 0);
    glDeleteRenderbuffers(1, &m_depth_buffer);
    glDeleteRenderbuffers(1, &m_stencil_buffer);
    m_stencil_buffer = 0;
    attach_renderbuffer(m_depth_buffer, depth24_stencil8, GL_DEPTH_ATTACHMENT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_depth_buffer);
    status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  }

  glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(old_framebuffer));

  if (status != GL_FRAMEBUFFER_COMPLETE) {
    destroy();
    throw std::runtime_error(std::string("Framebuffer: incomplete: ") + framebuffer_status_string(status));
  }

  WST_CHECK_GL("creating framebuffer");
}

Framebuffer::~Framebuffer()
{
  destroy();
}

void
Framebuffer::destroy()
{
  if (m_handle) {
    glDeleteFramebuffers(1, &m_handle);
    m_handle = 0;
  }
  if (m_depth_buffer) {
    glDeleteRenderbuffers(1, &m_depth_buffer);
    m_depth_buffer = 0;
  }
  if (m_stencil_buffer) {
    glDeleteRenderbuffers(1, &m_stencil_buffer);
    m_stencil_buffer = 0;
  }
}

} // namespace wstdisplay

/* EOF */
