// SPDX-FileCopyrightText: 2002-2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <wstdisplay/shader_program.hpp>

#include <stdexcept>
#include <string>
#include <vector>

#include <logmich/log.hpp>

#include "gl.hpp"

namespace wstdisplay {

namespace {

char const gles2_vert_preamble[] =
  "#version 100\n"
  "precision highp float;\n"
  "#line 1\n";

char const gles2_frag_preamble[] =
  "#version 100\n"
  "#ifdef GL_FRAGMENT_PRECISION_HIGH\n"
  "precision highp float;\n"
  "#else\n"
  "precision mediump float;\n"
  "#endif\n"
  "#line 1\n";

char const gl33_vert_preamble[] =
  "#version 330 core\n"
  "#define attribute in\n"
  "#define varying out\n"
  "#define texture2D texture\n"
  "#line 1\n";

char const gl33_frag_preamble[] =
  "#version 330 core\n"
  "#define varying in\n"
  "#define texture2D texture\n"
  "#define gl_FragColor wst_FragColor\n"
  "out vec4 wst_FragColor;\n"
  "#line 1\n";

GLuint compile(bool gles, GLenum type, std::string_view source)
{
  char const* preamble = nullptr;
  if (type == GL_VERTEX_SHADER) {
    preamble = gles ? gles2_vert_preamble : gl33_vert_preamble;
  } else {
    preamble = gles ? gles2_frag_preamble : gl33_frag_preamble;
  }

  GLuint shader = glCreateShader(type);
  char const* sources[] = { preamble, source.data() };
  GLint const lengths[] = { -1, static_cast<GLint>(source.size()) };
  glShaderSource(shader, 2, sources, lengths);
  glCompileShader(shader);

  GLint status = GL_FALSE;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
  if (status != GL_TRUE) {
    GLint length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
    std::vector<char> log(static_cast<size_t>(std::max(length, 1)));
    glGetShaderInfoLog(shader, static_cast<GLsizei>(log.size()), nullptr, log.data());
    glDeleteShader(shader);
    throw std::runtime_error(std::string("ShaderProgram: failed to compile ") +
                             (type == GL_VERTEX_SHADER ? "vertex" : "fragment") +
                             " shader:\n" + log.data());
  }

  return shader;
}

} // namespace

ShaderProgram::ShaderProgram(Caps const& caps, std::string_view vert_source, std::string_view frag_source) :
  m_handle(0),
  m_mvp_location(-1),
  m_texture_locations(),
  m_uniform_locations()
{
  GLuint const vert = compile(caps.is_gles(), GL_VERTEX_SHADER, vert_source);
  GLuint frag = 0;
  try {
    frag = compile(caps.is_gles(), GL_FRAGMENT_SHADER, frag_source);
  } catch (...) {
    glDeleteShader(vert);
    throw;
  }

  m_handle = glCreateProgram();
  glAttachShader(m_handle, vert);
  glAttachShader(m_handle, frag);

  glBindAttribLocation(m_handle, static_cast<GLuint>(Attrib::Position), "a_position");
  glBindAttribLocation(m_handle, static_cast<GLuint>(Attrib::Texcoord), "a_texcoord");
  glBindAttribLocation(m_handle, static_cast<GLuint>(Attrib::Color), "a_color");
  glBindAttribLocation(m_handle, static_cast<GLuint>(Attrib::Normal), "a_normal");
  glBindAttribLocation(m_handle, static_cast<GLuint>(Attrib::Effect), "a_effect");

  glLinkProgram(m_handle);
  glDeleteShader(vert);
  glDeleteShader(frag);

  GLint status = GL_FALSE;
  glGetProgramiv(m_handle, GL_LINK_STATUS, &status);
  if (status != GL_TRUE) {
    GLint length = 0;
    glGetProgramiv(m_handle, GL_INFO_LOG_LENGTH, &length);
    std::vector<char> log(static_cast<size_t>(std::max(length, 1)));
    glGetProgramInfoLog(m_handle, static_cast<GLsizei>(log.size()), nullptr, log.data());
    glDeleteProgram(m_handle);
    throw std::runtime_error(std::string("ShaderProgram: failed to link:\n") + log.data());
  }

  m_mvp_location = glGetUniformLocation(m_handle, "u_mvp");

  // samplers are bound to fixed units once
  glUseProgram(m_handle);
  for (int unit = 0; unit < max_textures; ++unit) {
    std::string const name = "u_texture" + std::to_string(unit);
    m_texture_locations[unit] = glGetUniformLocation(m_handle, name.c_str());
    if (m_texture_locations[unit] != -1) {
      glUniform1i(m_texture_locations[unit], unit);
    }
  }
  glUseProgram(0);

  WST_CHECK_GL("linking shader program");
}

ShaderProgram::~ShaderProgram()
{
  glDeleteProgram(m_handle);
}

int
ShaderProgram::get_uniform_location(std::string_view name) const
{
  auto it = m_uniform_locations.find(name);
  if (it != m_uniform_locations.end()) {
    return it->second;
  }

  int const location = glGetUniformLocation(m_handle, std::string(name).c_str());
  if (location == -1) {
    log_debug("ShaderProgram: no active uniform '{}'", name);
  }
  m_uniform_locations.emplace(std::string(name), location);
  return location;
}

} // namespace wstdisplay

/* EOF */
