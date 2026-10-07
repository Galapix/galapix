// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef HEADER_WSTDISPLAY_DEVICE_IMPL_HPP
#define HEADER_WSTDISPLAY_DEVICE_IMPL_HPP

// Internal header, shared by Device and Renderer

#include <wstdisplay/caps.hpp>
#include <wstdisplay/device.hpp>
#include <wstdisplay/framebuffer.hpp>
#include <wstdisplay/material.hpp>
#include <wstdisplay/mesh.hpp>
#include <wstdisplay/resource_pool.hpp>
#include <wstdisplay/shader_program.hpp>
#include <wstdisplay/static_batch.hpp>
#include <wstdisplay/texture.hpp>

namespace wstdisplay {

class Device::Impl final
{
public:
  explicit Impl(Caps caps_) :
    caps(std::move(caps_)),
    shutting_down(false),
    textures(),
    framebuffers(),
    programs(),
    materials(),
    meshes(),
    static_batches()
  {}

  template<typename T> ResourcePool<T>& pool();

  Caps caps;

  /** Set while the device is going away, destroy() is a no-op then */
  bool shutting_down;

  ResourcePool<Texture> textures;
  ResourcePool<Framebuffer> framebuffers;
  ResourcePool<ShaderProgram> programs;
  ResourcePool<Material> materials;
  ResourcePool<Mesh> meshes;
  ResourcePool<StaticBatch> static_batches;

public:
  Impl(Impl const&) = delete;
  Impl& operator=(Impl const&) = delete;
};

template<> inline ResourcePool<Texture>& Device::Impl::pool<Texture>() { return textures; }
template<> inline ResourcePool<Framebuffer>& Device::Impl::pool<Framebuffer>() { return framebuffers; }
template<> inline ResourcePool<ShaderProgram>& Device::Impl::pool<ShaderProgram>() { return programs; }
template<> inline ResourcePool<Material>& Device::Impl::pool<Material>() { return materials; }
template<> inline ResourcePool<Mesh>& Device::Impl::pool<Mesh>() { return meshes; }
template<> inline ResourcePool<StaticBatch>& Device::Impl::pool<StaticBatch>() { return static_batches; }

} // namespace wstdisplay

#endif

/* EOF */
