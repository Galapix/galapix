// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <wstdisplay/device.hpp>

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "device_impl.hpp"
#include "gl.hpp"

namespace wstdisplay {

namespace {

std::string read_file(std::filesystem::path const& filename)
{
  std::ifstream in(filename, std::ios::binary);
  if (!in) {
    throw std::runtime_error("couldn't open " + filename.string());
  }
  std::ostringstream str;
  str << in.rdbuf();
  return str.str();
}

template<typename T> char const* resource_name();
template<> char const* resource_name<Texture>() { return "Texture"; }
template<> char const* resource_name<Framebuffer>() { return "Framebuffer"; }
template<> char const* resource_name<ShaderProgram>() { return "ShaderProgram"; }
template<> char const* resource_name<Material>() { return "Material"; }
template<> char const* resource_name<Mesh>() { return "Mesh"; }
template<> char const* resource_name<StaticBatch>() { return "StaticBatch"; }

} // namespace

Device::Device(ProcLoader loader) :
  m_impl(std::make_unique<Impl>(detail::load_gl(loader)))
{
}

Device::~Device()
{
  m_impl->shutting_down = true;

  // users of other resources first
  m_impl->static_batches.clear();
  m_impl->meshes.clear();
  m_impl->materials.clear();
  m_impl->framebuffers.clear();
  m_impl->programs.clear();
  m_impl->textures.clear();
}

Caps const&
Device::get_caps() const
{
  return m_impl->caps;
}

Unique<Texture>
Device::create_texture(surf::SoftwareSurface const& image, TextureParams const& params)
{
  auto texture = std::make_unique<Texture>(m_impl->caps, image.get_size(), params);
  texture->put(image, geom::ipoint(0, 0));
  return Unique<Texture>(*this, m_impl->textures.insert(std::move(texture)));
}

Unique<Texture>
Device::create_texture(geom::isize const& size, TextureParams const& params)
{
  return Unique<Texture>(*this, m_impl->textures.insert(std::make_unique<Texture>(m_impl->caps, size, params)));
}

Unique<Framebuffer>
Device::create_framebuffer(geom::isize const& size, FramebufferParams const& params)
{
  return Unique<Framebuffer>(*this, m_impl->framebuffers.insert(std::make_unique<Framebuffer>(*this, size, params)));
}

Unique<ShaderProgram>
Device::create_program(std::string_view vert_source, std::string_view frag_source)
{
  return Unique<ShaderProgram>(*this, m_impl->programs.insert(
                                 std::make_unique<ShaderProgram>(m_impl->caps, vert_source, frag_source)));
}

Unique<ShaderProgram>
Device::load_program(std::filesystem::path const& vert_filename,
                     std::filesystem::path const& frag_filename)
{
  try {
    return create_program(read_file(vert_filename), read_file(frag_filename));
  } catch (std::exception const& err) {
    throw std::runtime_error(vert_filename.string() + ", " + frag_filename.string() + ": " + err.what());
  }
}

Unique<Material>
Device::create_material(ProgramId program)
{
  return Unique<Material>(*this, m_impl->materials.insert(std::make_unique<Material>(*this, program)));
}

Unique<Mesh>
Device::create_mesh()
{
  return Unique<Mesh>(*this, m_impl->meshes.insert(std::make_unique<Mesh>()));
}

Unique<StaticBatch>
Device::create_static_batch()
{
  return Unique<StaticBatch>(*this, m_impl->static_batches.insert(std::make_unique<StaticBatch>()));
}

template<typename T>
T*
Device::find(Handle<T> handle)
{
  return m_impl->pool<T>().find(handle);
}

template<typename T>
T const*
Device::find(Handle<T> handle) const
{
  return m_impl->pool<T>().find(handle);
}

template<typename T>
T&
Device::get(Handle<T> handle)
{
  T* const object = find(handle);
  if (object == nullptr) {
    throw std::invalid_argument(std::string("Device::get: ") + (handle ? "stale " : "null ") +
                                resource_name<T>() + " handle");
  }
  return *object;
}

template<typename T>
T const&
Device::get(Handle<T> handle) const
{
  return const_cast<Device&>(*this).get(handle);
}

template<typename T>
void
Device::destroy(Handle<T> handle)
{
  if (!m_impl->shutting_down) {
    m_impl->pool<T>().destroy(handle);
  }
}

template<typename T>
size_t
Device::count() const
{
  return m_impl->pool<T>().count();
}

void
Device::collect()
{
  // repeat as deleting can destroy further resources, e.g. the
  // texture of a framebuffer
  bool deleted = true;
  while (deleted) {
    deleted = false;
    deleted |= m_impl->static_batches.collect();
    deleted |= m_impl->meshes.collect();
    deleted |= m_impl->materials.collect();
    deleted |= m_impl->framebuffers.collect();
    deleted |= m_impl->programs.collect();
    deleted |= m_impl->textures.collect();
  }
}

#define WST_INSTANTIATE_DEVICE(T)                                       \
  template T* Device::find<T>(Handle<T>);                               \
  template T const* Device::find<T>(Handle<T>) const;                   \
  template T& Device::get<T>(Handle<T>);                                \
  template T const& Device::get<T>(Handle<T>) const;                    \
  template void Device::destroy<T>(Handle<T>);                          \
  template size_t Device::count<T>() const

WST_INSTANTIATE_DEVICE(Texture);
WST_INSTANTIATE_DEVICE(Framebuffer);
WST_INSTANTIATE_DEVICE(ShaderProgram);
WST_INSTANTIATE_DEVICE(Material);
WST_INSTANTIATE_DEVICE(Mesh);
WST_INSTANTIATE_DEVICE(StaticBatch);

#undef WST_INSTANTIATE_DEVICE

} // namespace wstdisplay

/* EOF */
