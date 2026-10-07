// SPDX-FileCopyrightText: 2002-2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <wstdisplay/texture.hpp>

#include <cstring>
#include <stdexcept>
#include <vector>

#include <logmich/log.hpp>
#include <surf/convert.hpp>
#include <surf/software_surface.hpp>

#include <wstdisplay/caps.hpp>
#include "gl.hpp"

namespace wstdisplay {

namespace {

bool is_power_of_two(int v)
{
  return v > 0 && (v & (v - 1)) == 0;
}

/** Image data in a format GLES 2.0 can take directly: tightly packed
    RGB8 or RGBA8 rows (GLES 2.0 has no GL_UNPACK_ROW_LENGTH) */
struct UploadData
{
  GLenum format;
  geom::isize size;
  std::vector<uint8_t> storage;
  void const* pixels;
};

UploadData prepare_upload(surf::SoftwareSurface const& image)
{
  surf::SoftwareSurface converted;
  surf::SoftwareSurface const* src = &image;

  if (image.get_format() != surf::PixelFormat::RGBA8 &&
      image.get_format() != surf::PixelFormat::RGB8) {
    converted = surf::convert(image, surf::PixelFormat::RGBA8);
    src = &converted;
  }

  int const bpp = (src->get_format() == surf::PixelFormat::RGB8) ? 3 : 4;
  size_t const row_size = static_cast<size_t>(src->get_width()) * static_cast<size_t>(bpp);

  UploadData data{
    .format = (bpp == 3) ? static_cast<GLenum>(GL_RGB) : static_cast<GLenum>(GL_RGBA),
    .size = src->get_size(),
    .storage = {},
    .pixels = nullptr
  };

  // always copy, the converted surface would go out of scope otherwise
  data.storage.resize(row_size * static_cast<size_t>(src->get_height()));
  for (int y = 0; y < src->get_height(); ++y) {
    std::memcpy(data.storage.data() + row_size * static_cast<size_t>(y),
                src->get_row_data(y), row_size);
  }
  data.pixels = data.storage.data();
  return data;
}

GLint to_gl(TextureFilter filter)
{
  return filter == TextureFilter::Nearest ? GL_NEAREST : GL_LINEAR;
}

GLint to_gl(TextureWrap wrap)
{
  return wrap == TextureWrap::Repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE;
}

} // namespace

Texture::Texture(Caps const& caps, geom::isize const& size, TextureParams const& params) :
  m_handle(0),
  m_size(size),
  m_params(params)
{
  if (size.width() <= 0 || size.height() <= 0) {
    throw std::invalid_argument("Texture: invalid size");
  }

  if (size.width() > caps.max_texture_size || size.height() > caps.max_texture_size) {
    log_warn("Texture: size {}x{} exceeds GL_MAX_TEXTURE_SIZE {}",
             size.width(), size.height(), caps.max_texture_size);
  }

  bool const repeat = (m_params.wrap_x == TextureWrap::Repeat || m_params.wrap_y == TextureWrap::Repeat);
  if (repeat && !caps.npot_repeat &&
      (!is_power_of_two(size.width()) || !is_power_of_two(size.height()))) {
    log_warn("Texture: GL_REPEAT on a non-power-of-two texture ({}x{}) is not supported, using clamp",
             size.width(), size.height());
    m_params.wrap_x = TextureWrap::Clamp;
    m_params.wrap_y = TextureWrap::Clamp;
  }

  glGenTextures(1, &m_handle);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, m_handle);

  // GLES 2.0 requires internalformat == format
  std::vector<uint8_t> zeros(static_cast<size_t>(size.width()) * static_cast<size_t>(size.height()) * 4, 0);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.width(), size.height(), 0,
               GL_RGBA, GL_UNSIGNED_BYTE, zeros.data());

  apply_params();
  WST_CHECK_GL("creating texture");
}

Texture::~Texture()
{
  glDeleteTextures(1, &m_handle);
}

void
Texture::apply_params()
{
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, m_handle);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, to_gl(m_params.filter));
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, to_gl(m_params.filter));
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, to_gl(m_params.wrap_x));
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, to_gl(m_params.wrap_y));
}

void
Texture::set_filter(TextureFilter filter)
{
  m_params.filter = filter;
  apply_params();
}

void
Texture::set_wrap(TextureWrap wrap_x, TextureWrap wrap_y)
{
  m_params.wrap_x = wrap_x;
  m_params.wrap_y = wrap_y;
  apply_params();
}

void
Texture::put(surf::SoftwareSurface const& image, geom::ipoint const& pos)
{
  put(image, geom::irect(image.get_size()), pos);
}

void
Texture::put(surf::SoftwareSurface const& image, geom::irect const& srcrect, geom::ipoint const& pos)
{
  if (srcrect.width() <= 0 || srcrect.height() <= 0) {
    return;
  }

  if (pos.x() < 0 || pos.y() < 0 ||
      pos.x() + srcrect.width() > m_size.width() ||
      pos.y() + srcrect.height() > m_size.height()) {
    throw std::out_of_range("Texture::put: region outside of texture");
  }

  UploadData const data = (srcrect == geom::irect(image.get_size())) ?
    prepare_upload(image) :
    prepare_upload(image.get_view(srcrect));

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, m_handle);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexSubImage2D(GL_TEXTURE_2D, 0, pos.x(), pos.y(),
                  data.size.width(), data.size.height(),
                  data.format, GL_UNSIGNED_BYTE, data.pixels);
  WST_CHECK_GL("uploading texture data");
}

surf::SoftwareSurface
Texture::get_software_surface() const
{
  // glGetTexImage() doesn't exist on GLES, read back through a framebuffer instead
  GLint old_framebuffer = 0;
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &old_framebuffer);

  GLuint framebuffer = 0;
  glGenFramebuffers(1, &framebuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_handle, 0);

  surf::SoftwareSurface surface = surf::SoftwareSurface::create(surf::PixelFormat::RGBA8, m_size);
  std::vector<uint8_t> pixels(static_cast<size_t>(m_size.width()) * static_cast<size_t>(m_size.height()) * 4);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(0, 0, m_size.width(), m_size.height(), GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

  glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(old_framebuffer));
  glDeleteFramebuffers(1, &framebuffer);
  WST_CHECK_GL("reading back texture");

  // texture row 0 is the first row of the uploaded image
  size_t const row_size = static_cast<size_t>(m_size.width()) * 4;
  for (int y = 0; y < m_size.height(); ++y) {
    std::memcpy(surface.get_row_data(y), pixels.data() + row_size * static_cast<size_t>(y), row_size);
  }
  return surface;
}

} // namespace wstdisplay

/* EOF */
