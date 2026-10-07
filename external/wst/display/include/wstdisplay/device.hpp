// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef HEADER_WSTDISPLAY_DEVICE_HPP
#define HEADER_WSTDISPLAY_DEVICE_HPP

#include <filesystem>
#include <memory>
#include <string_view>
#include <utility>

#include <geom/size.hpp>
#include <surf/software_surface.hpp>

#include "caps.hpp"
#include "handle.hpp"
#include "texture_params.hpp"

namespace wstdisplay {

template<typename T> class Unique;

struct FramebufferParams
{
  TextureParams texture = {};

  /** Attach a depth buffer, needed for Mesh depth testing */
  bool depth = false;

  /** Attach a stencil buffer */
  bool stencil = false;
};

/** Owns all GPU resources of an OpenGL context: textures,
    framebuffers, shader programs, materials, meshes and static
    batches. Everything else refers to them by Handle.

    Resources are created through the create_*() functions, which
    return a Unique owner, and are looked up with get(). destroy()
    only marks a resource, it stays usable until collect() runs at the
    end of the frame (Renderer::end_frame()), so a canvas that was
    recorded before the resource was destroyed still renders
    correctly. Afterwards its handles are stale: find() returns
    nullptr, get() throws and the renderer skips draws using them.

    The Device must outlive every Unique handle created from it. */
class Device final
{
public:
  using ProcLoader = void* (*)(char const* name);

public:
  /** Initialize on the current OpenGL context, \a loader resolves
      GL function names, e.g. SDL_GL_GetProcAddress */
  explicit Device(ProcLoader loader);
  ~Device();

  Caps const& get_caps() const;

  /** Upload \a image, any pixel format is accepted */
  [[nodiscard]] Unique<Texture> create_texture(surf::SoftwareSurface const& image, TextureParams const& params = {});

  /** A transparent RGBA8 texture */
  [[nodiscard]] Unique<Texture> create_texture(geom::isize const& size, TextureParams const& params = {});

  /** An offscreen render target, its color texture is owned by the
      framebuffer, see Framebuffer::get_texture() */
  [[nodiscard]] Unique<Framebuffer> create_framebuffer(geom::isize const& size, FramebufferParams const& params = {});

  /** Compile and link a shader program, see ShaderProgram for the
      GLSL dialect. Throws on compile or link errors. */
  [[nodiscard]] Unique<ShaderProgram> create_program(std::string_view vert_source, std::string_view frag_source);
  [[nodiscard]] Unique<ShaderProgram> load_program(std::filesystem::path const& vert_filename,
                                                   std::filesystem::path const& frag_filename);

  /** A material using \a program, which must outlive it */
  [[nodiscard]] Unique<Material> create_material(ProgramId program);

  [[nodiscard]] Unique<Mesh> create_mesh();
  [[nodiscard]] Unique<StaticBatch> create_static_batch();

  /** The resource \a handle refers to, nullptr if the handle is null
      or stale */
  template<typename T> T* find(Handle<T> handle);
  template<typename T> T const* find(Handle<T> handle) const;

  /** The resource \a handle refers to, throws std::invalid_argument
      if the handle is null or stale */
  template<typename T> T& get(Handle<T> handle);
  template<typename T> T const& get(Handle<T> handle) const;

  /** Mark the resource for deletion at the next collect(), null and
      stale handles are ignored */
  template<typename T> void destroy(Handle<T> handle);

  /** Delete the resources destroyed since the last call, called by
      Renderer::end_frame() */
  void collect();

  /** Number of live resources of type T, for leak checks */
  template<typename T> size_t count() const;

private:
  friend class Renderer;
  class Impl;
  std::unique_ptr<Impl> m_impl;

public:
  Device(Device const&) = delete;
  Device& operator=(Device const&) = delete;
};

/** Unique ownership of a Device resource, like std::unique_ptr: the
    resource is destroyed when the owner goes away. Converts
    implicitly to the plain Handle, which is what gets passed around
    and stored. */
template<typename T>
class Unique final
{
public:
  Unique() = default;
  Unique(Device& device, Handle<T> handle) : m_device(&device), m_handle(handle) {}

  Unique(Unique&& other) noexcept :
    m_device(std::exchange(other.m_device, nullptr)),
    m_handle(std::exchange(other.m_handle, {}))
  {}

  Unique& operator=(Unique&& other) noexcept
  {
    if (this != &other) {
      reset();
      m_device = std::exchange(other.m_device, nullptr);
      m_handle = std::exchange(other.m_handle, {});
    }
    return *this;
  }

  ~Unique() { reset(); }

  Handle<T> get() const { return m_handle; }
  operator Handle<T>() const { return m_handle; }
  explicit operator bool() const { return static_cast<bool>(m_handle); }

  T& operator*() const { return m_device->get(m_handle); }
  T* operator->() const { return &m_device->get(m_handle); }

  /** Give up ownership without destroying the resource */
  Handle<T> release()
  {
    m_device = nullptr;
    return std::exchange(m_handle, {});
  }

  void reset()
  {
    if (m_device && m_handle) {
      m_device->destroy(m_handle);
    }
    m_device = nullptr;
    m_handle = {};
  }

private:
  Device* m_device = nullptr;
  Handle<T> m_handle = {};

public:
  Unique(Unique const&) = delete;
  Unique& operator=(Unique const&) = delete;
};

} // namespace wstdisplay

#endif

/* EOF */
