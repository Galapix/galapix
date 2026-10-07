// SPDX-FileCopyrightText: 2002-2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef HEADER_WSTDISPLAY_SHADER_PROGRAM_HPP
#define HEADER_WSTDISPLAY_SHADER_PROGRAM_HPP

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <string_view>

#include "fwd.hpp"

namespace wstdisplay {

/** Fixed attribute locations, bound before linking, so every program
    can be fed from the same vertex layout */
enum class Attrib : uint32_t
{
  Position = 0, // vec2 a_position (vec3 for meshes)
  Texcoord = 1, // vec2 a_texcoord
  Color = 2,    // vec4 a_color
  Normal = 3,   // vec3 a_normal, meshes only
  Effect = 4    // vec4 a_effect, 2D only, see Vertex::effect
};

/** A linked vertex+fragment shader, owned by a Device and created
    with Device::create_program().

    Shaders are written in GLSL ES 1.00 (the GLES 2.0 dialect) without
    a #version line, they are translated to GLSL 3.30 core on desktop
    GL by a prepended preamble. Use 'attribute', 'varying',
    'texture2D()' and 'gl_FragColor' as in GLSL ES 1.00.

    Built-in names:
    - attributes: a_position, a_texcoord, a_color, a_effect, a_normal
    - uniforms: u_mvp (mat4), u_texture0 .. u_texture3 (sampler2D) */
class ShaderProgram final
{
public:
  static constexpr int max_textures = 4;

  ShaderProgram(Caps const& caps, std::string_view vert_source, std::string_view frag_source);
  ~ShaderProgram();

  /** Location of the uniform \a name or -1, results are cached */
  int get_uniform_location(std::string_view name) const;

  int get_mvp_location() const { return m_mvp_location; }
  int get_texture_location(int unit) const { return m_texture_locations[unit]; }

  uint32_t get_handle() const { return m_handle; }

private:
  uint32_t m_handle;
  int m_mvp_location;
  int m_texture_locations[max_textures];
  mutable std::map<std::string, int, std::less<>> m_uniform_locations;

public:
  ShaderProgram(ShaderProgram const&) = delete;
  ShaderProgram& operator=(ShaderProgram const&) = delete;
};

} // namespace wstdisplay

#endif

/* EOF */
