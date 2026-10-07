// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <wstdisplay/material.hpp>

#include <algorithm>
#include <stdexcept>

#include <glm/gtc/type_ptr.hpp>

#include <wstdisplay/device.hpp>

namespace wstdisplay {

Material::Material(Device& device, ProgramId program) :
  m_device(device),
  m_program(program),
  m_textures(),
  m_uniforms()
{
  // validate early
  m_device.get(m_program);
}

void
Material::set_texture(int unit, TextureId texture)
{
  if (unit < 1 || unit >= ShaderProgram::max_textures) {
    throw std::out_of_range("Material::set_texture: unit out of range");
  }
  m_textures[static_cast<size_t>(unit)] = texture;
}

void
Material::set(std::string_view name, UniformType type, float const* data, size_t count)
{
  int const location = m_device.get(m_program).get_uniform_location(name);
  if (location == -1) {
    return;
  }

  auto it = std::find_if(m_uniforms.begin(), m_uniforms.end(),
                         [location](Uniform const& u) { return u.location == location; });
  if (it == m_uniforms.end()) {
    m_uniforms.push_back(Uniform{location, type, {}});
    it = m_uniforms.end() - 1;
  }

  it->type = type;
  std::copy_n(data, count, it->data.begin());
}

void
Material::set_uniform(std::string_view name, int value)
{
  float const v = static_cast<float>(value);
  set(name, UniformType::Int, &v, 1);
}

void
Material::set_uniform(std::string_view name, float value)
{
  set(name, UniformType::Float, &value, 1);
}

void
Material::set_uniform(std::string_view name, glm::vec2 const& value)
{
  set(name, UniformType::Vec2, glm::value_ptr(value), 2);
}

void
Material::set_uniform(std::string_view name, glm::vec3 const& value)
{
  set(name, UniformType::Vec3, glm::value_ptr(value), 3);
}

void
Material::set_uniform(std::string_view name, glm::vec4 const& value)
{
  set(name, UniformType::Vec4, glm::value_ptr(value), 4);
}

void
Material::set_uniform(std::string_view name, glm::mat4 const& value)
{
  set(name, UniformType::Mat4, glm::value_ptr(value), 16);
}

} // namespace wstdisplay

/* EOF */
