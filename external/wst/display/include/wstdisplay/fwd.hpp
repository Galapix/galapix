// SPDX-FileCopyrightText: 2002-2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef HEADER_WSTDISPLAY_FWD_HPP
#define HEADER_WSTDISPLAY_FWD_HPP

#include "handle.hpp"

namespace wstdisplay {

class BitmapFont;
class Canvas;
class Compositor;
class Device;
class Font;
class FontManager;
class Framebuffer;
class Material;
class Mesh;
class OpenGLWindow;
class Renderer;
class ShaderProgram;
class StaticBatch;
class Surface;
class SurfaceManager;
class TextArea;
class Texture;
class TextureManager;
class TexturePacker;
class View;

struct Caps;
struct DrawParams;
struct FontEffect;
struct FramebufferParams;
struct RenderPass;
struct RenderStats;

struct TextureParams;
struct Vertex;

template<typename T> class Unique;

} // namespace wstdisplay

#endif

/* EOF */
