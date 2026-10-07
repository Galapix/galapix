// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef HEADER_WSTDISPLAY_MATERIAL_HPP
#define HEADER_WSTDISPLAY_MATERIAL_HPP

#include <array>
#include <cstdint>
#include <string_view>
#include <vector>

#include <glm/glm.hpp>

#include "fwd.hpp"
#include "handle.hpp"
#include "shader_program.hpp"

namespace wstdisplay {

/** A custom shader together with its extra textures and uniform
    values, owned by a Device and created with
    Device::create_material(). Drawing with a material replaces the
    default program, texture unit 0 is still the texture of the drawn
    surface.

    Draws sharing the same Material can be batched together, so share
    materials instead of creating one per draw. Per draw parameters
    can be passed through Vertex::effect (DrawParams::effect). */
class Material final
{
public:
  enum class UniformType : uint8_t { Int, Float, Vec2, Vec3, Vec4, Mat4 };

  struct Uniform
  {
    int location;
    UniformType type;
    std::array<float, 16> data;
  };

public:
  Material(Device& device, ProgramId program);

  ProgramId get_program() const { return m_program; }

  /** Set the texture for \a unit in [1, ShaderProgram::max_textures),
      the material doesn't own it */
  void set_texture(int unit, TextureId texture);
  TextureId get_texture(int unit) const { return m_textures[static_cast<size_t>(unit)]; }

  /** Set the uniform \a name. Names the program doesn't have are
      ignored, as the GLSL compiler removes unused uniforms, and
      logged once at debug level. */
  void set_uniform(std::string_view name, int value);
  void set_uniform(std::string_view name, float value);
  void set_uniform(std::string_view name, glm::vec2 const& value);
  void set_uniform(std::string_view name, glm::vec3 const& value);
  void set_uniform(std::string_view name, glm::vec4 const& value);
  void set_uniform(std::string_view name, glm::mat4 const& value);

  std::vector<Uniform> const& get_uniforms() const { return m_uniforms; }

private:
  void set(std::string_view name, UniformType type, float const* data, size_t count);

private:
  Device& m_device;
  ProgramId m_program;
  std::array<TextureId, ShaderProgram::max_textures> m_textures;
  std::vector<Uniform> m_uniforms;

public:
  Material(Material const&) = delete;
  Material& operator=(Material const&) = delete;
};

} // namespace wstdisplay

#endif

/* EOF */
