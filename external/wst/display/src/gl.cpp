// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "gl.hpp"

#include <cstring>
#include <sstream>
#include <stdexcept>

#include <logmich/log.hpp>

namespace wstdisplay::detail {

namespace {

std::string get_gl_string(GLenum name)
{
  GLubyte const* ret = glGetString(name);
  return ret ? reinterpret_cast<char const*>(ret) : "";
}

bool has_extension(GLApi api, std::string const& extensions, char const* name)
{
  if (api == GLApi::GL33) {
    GLint count = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &count);
    for (GLint i = 0; i < count; ++i) {
      GLubyte const* ext = glGetStringi(GL_EXTENSIONS, static_cast<GLuint>(i));
      if (ext && std::strcmp(reinterpret_cast<char const*>(ext), name) == 0) {
        return true;
      }
    }
    return false;
  } else {
    // whole-word match in the space separated list
    size_t const len = std::strlen(name);
    size_t pos = 0;
    while ((pos = extensions.find(name, pos)) != std::string::npos) {
      bool const start_ok = (pos == 0 || extensions[pos - 1] == ' ');
      bool const end_ok = (pos + len == extensions.size() || extensions[pos + len] == ' ');
      if (start_ok && end_ok) {
        return true;
      }
      pos += len;
    }
    return false;
  }
}

/* gladLoadGL() only loads the desktop GL entry points up to the
   version number it parses from a "OpenGL ES x.y" string. A real
   GLES 2.0 context thus lacks the framebuffer functions, which are
   GL 3.0 on desktop but core in GLES 2.0, so load them by hand. */
template<typename T>
void load_missing(T& func, void* (*loader)(char const* name), char const* name)
{
  if (func == nullptr) {
    func = reinterpret_cast<T>(loader(name));
    if (func == nullptr) {
      log_warn("GLES: failed to load {}", name);
    }
  }
}

void load_gles2_functions(void* (*loader)(char const* name))
{
  load_missing(glad_glBindFramebuffer, loader, "glBindFramebuffer");
  load_missing(glad_glBindRenderbuffer, loader, "glBindRenderbuffer");
  load_missing(glad_glCheckFramebufferStatus, loader, "glCheckFramebufferStatus");
  load_missing(glad_glDeleteFramebuffers, loader, "glDeleteFramebuffers");
  load_missing(glad_glDeleteRenderbuffers, loader, "glDeleteRenderbuffers");
  load_missing(glad_glFramebufferRenderbuffer, loader, "glFramebufferRenderbuffer");
  load_missing(glad_glFramebufferTexture2D, loader, "glFramebufferTexture2D");
  load_missing(glad_glGenFramebuffers, loader, "glGenFramebuffers");
  load_missing(glad_glGenRenderbuffers, loader, "glGenRenderbuffers");
  load_missing(glad_glGenerateMipmap, loader, "glGenerateMipmap");
  load_missing(glad_glRenderbufferStorage, loader, "glRenderbufferStorage");
}

} // namespace

char const* gl_error_string(GLenum error)
{
  switch (error)
  {
    case GL_NO_ERROR: return "GL_NO_ERROR";
    case GL_INVALID_ENUM: return "GL_INVALID_ENUM";
    case GL_INVALID_VALUE: return "GL_INVALID_VALUE";
    case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION";
    case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
    case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY";
    default: return "unknown GL error";
  }
}

void check_gl(char const* file, int line, char const* what)
{
  GLenum const error = glGetError();
  if (error != GL_NO_ERROR) {
    std::ostringstream msg;
    msg << file << ":" << line << ": OpenGL error while " << what << ": " << gl_error_string(error);
    throw std::runtime_error(msg.str());
  }
}

Caps load_gl(void* (*loader)(char const* name))
{
  if (!gladLoadGL(reinterpret_cast<GLADloadfunc>(loader))) {
    throw std::runtime_error("failed to load OpenGL functions");
  }

  Caps caps;
  caps.vendor = get_gl_string(GL_VENDOR);
  caps.renderer = get_gl_string(GL_RENDERER);
  caps.version = get_gl_string(GL_VERSION);
  caps.glsl_version = get_gl_string(GL_SHADING_LANGUAGE_VERSION);
  caps.api = (caps.version.starts_with("OpenGL ES")) ? GLApi::GLES2 : GLApi::GL33;

  std::string extensions;
  if (caps.api == GLApi::GLES2) {
    load_gles2_functions(loader);
    extensions = get_gl_string(GL_EXTENSIONS);
  }

  GLint max_texture_size = 0;
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
  caps.max_texture_size = max_texture_size;

  if (caps.api == GLApi::GLES2) {
    // GLES 3.x contexts have full NPOT support as well
    caps.npot_repeat = caps.version.starts_with("OpenGL ES 3") ||
      has_extension(caps.api, extensions, "GL_OES_texture_npot") ||
      has_extension(caps.api, extensions, "GL_ARB_texture_non_power_of_two");
    caps.read_buffer = false;
  } else {
    caps.npot_repeat = true;
    caps.read_buffer = true;
  }

  // flush errors from probing
  while (glGetError() != GL_NO_ERROR) {}

  log_info("OpenGL: {} / {} / {} / GLSL {}", caps.vendor, caps.renderer, caps.version, caps.glsl_version);
  log_info("OpenGL: api={} max_texture_size={} npot_repeat={}",
           caps.is_gles() ? "GLES2" : "GL33", caps.max_texture_size, caps.npot_repeat);

  return caps;
}

} // namespace wstdisplay::detail

/* EOF */
