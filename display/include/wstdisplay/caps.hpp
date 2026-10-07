// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef HEADER_WSTDISPLAY_CAPS_HPP
#define HEADER_WSTDISPLAY_CAPS_HPP

#include <string>

namespace wstdisplay {

/** The OpenGL flavor the context implements */
enum class GLApi
{
  /** OpenGL 3.3 core profile */
  GL33,

  /** OpenGL ES 2.0 (also used for ES 3.x and WebGL contexts) */
  GLES2
};

/** Capabilities of the current OpenGL context. The library treats
    GLES 2.0 as the baseline, everything beyond it is optional. */
struct Caps
{
  GLApi api = GLApi::GL33;

  std::string vendor = {};
  std::string renderer = {};
  std::string version = {};
  std::string glsl_version = {};

  int max_texture_size = 2048;

  /** Non-power-of-two textures with GL_REPEAT and mipmaps, always
      true on GL 3.3, on GLES 2.0 only with GL_OES_texture_npot */
  bool npot_repeat = true;

  /** glReadPixels() from GL_FRONT/GL_BACK and glGetTexImage(), not
      available on GLES */
  bool read_buffer = true;

  bool is_gles() const { return api == GLApi::GLES2; }
};

} // namespace wstdisplay

#endif

/* EOF */
