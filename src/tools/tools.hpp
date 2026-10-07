// Galapix - an image viewer for large image collections
// Copyright (C) 2008-2026 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#ifndef HEADER_GALAPIX_TOOLS_TOOLS_HPP
#define HEADER_GALAPIX_TOOLS_TOOLS_HPP

#include <cstdint>

#include <wstdisplay/fwd.hpp>

#include "math/vector2f.hpp"
#include "math/vector2i.hpp"

namespace galapix {

class Viewer;

/** What a mouse button or a held key does */
enum class ToolKind : std::uint8_t
{
  None,
  Pan,         ///< drag moves the view (always, in trackball mode)
  ZoomIn,      ///< zoom in toward the pointer while held
  ZoomOut,     ///< zoom out from the pointer while held
  ZoomRect,    ///< drag a rectangle, zoom to it
  Move,        ///< click toggles an image's selection and drags the selection,
               ///< dragging on empty space selects with a rectangle
  Resize,      ///< drag scales the selection around its center
  Rotate,      ///< drag sets the angle of the selected images
  Grid,        ///< drag a rectangle, it becomes the grid cell
  ViewRotate   ///< drag rotates the view around the window center
};

/** Tool sets of the tool keys (p, z, y, m, r) */
struct ToolSet
{
  ToolKind left;
  ToolKind middle;
  ToolKind right;
  char const* name;
};

inline constexpr ToolSet kPanTools        {ToolKind::ZoomIn,   ToolKind::Pan, ToolKind::ZoomOut, "Pan&Zoom"};
inline constexpr ToolSet kZoomRectTools   {ToolKind::ZoomRect, ToolKind::Pan, ToolKind::ZoomOut, "Zoom rect"};
inline constexpr ToolSet kGridTools       {ToolKind::Grid,     ToolKind::Pan, ToolKind::ZoomOut, "Grid"};
inline constexpr ToolSet kMoveResizeTools {ToolKind::Move,     ToolKind::Pan, ToolKind::Resize,  "Move&Resize"};
inline constexpr ToolSet kMoveRotateTools {ToolKind::Move,     ToolKind::Pan, ToolKind::Rotate,  "Move&Rotate"};

/** State of one input slot (a mouse button or a key) using a tool. Plain
    data; the functions below run the tool against the Viewer. */
struct ToolDrag
{
  ToolKind kind = ToolKind::None;
  /** button or key held */
  bool active = false;
  /** Move: rectangle selection instead of moving the selection */
  bool select_box = false;
  /** screen position, current and at press */
  Vector2i mouse_pos = Vector2i(0, 0);
  Vector2i press_pos = Vector2i(0, 0);
  /** world position at press */
  Vector2f press_world = Vector2f(0.0f, 0.0f);
  /** Resize, Rotate: selection center at press */
  Vector2f center = Vector2f(0.0f, 0.0f);
  /** Rotate, ViewRotate: angle at press / last move, radians */
  float start_angle = 0.0f;
  /** Resize: scale factor applied so far */
  float applied_scale = 1.0f;
};

void tool_down(Viewer& viewer, ToolDrag& drag, Vector2i const& pos);
void tool_move(Viewer& viewer, ToolDrag& drag, Vector2i const& pos, Vector2i const& rel);
void tool_up(Viewer& viewer, ToolDrag& drag, Vector2i const& pos);

/** Per frame, for tools acting while held (zooming) */
void tool_update(Viewer& viewer, ToolDrag& drag, Vector2i const& mouse_pos, float delta);

/** Rubber band rectangles in world space */
void tool_draw(Viewer& viewer, ToolDrag const& drag, wstdisplay::Canvas& canvas);

/** True while the slot changes the view every frame */
bool tool_animating(ToolDrag const& drag);

} // namespace galapix

#endif

/* EOF */
