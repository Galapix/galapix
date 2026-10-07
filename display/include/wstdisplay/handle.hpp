// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef HEADER_WSTDISPLAY_HANDLE_HPP
#define HEADER_WSTDISPLAY_HANDLE_HPP

#include <compare>
#include <cstdint>

namespace wstdisplay {

/** A reference to a resource of type T owned by a Device: a slot
    index plus the generation of the slot. The generation is bumped
    when the resource is deleted, so a handle that outlived its
    resource is detected instead of reaching whatever reuses the slot.

    Handles are trivially copyable 8 byte values without ownership,
    meant to be stored in components, commands and sprite data. A
    default constructed handle is null. */
template<typename T>
struct Handle
{
  uint32_t index = 0;
  uint32_t generation = 0;

  explicit operator bool() const { return generation != 0; }

  friend bool operator==(Handle const& lhs, Handle const& rhs) = default;
  friend auto operator<=>(Handle const& lhs, Handle const& rhs) = default;
};

class Font;
class Framebuffer;
class Material;
class Mesh;
class ShaderProgram;
class StaticBatch;
class Texture;

using FontId = Handle<Font>;
using FramebufferId = Handle<Framebuffer>;
using MaterialId = Handle<Material>;
using MeshId = Handle<Mesh>;
using ProgramId = Handle<ShaderProgram>;
using StaticBatchId = Handle<StaticBatch>;
using TextureId = Handle<Texture>;

} // namespace wstdisplay

#endif

/* EOF */
