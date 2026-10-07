// Galapix - an image viewer for large image collections
// Copyright (C) 2008-2026 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include "tools/tools.hpp"

#include <cmath>
#include <iostream>

#include <glm/gtc/constants.hpp>
#include <surf/color.hpp>
#include <wstdisplay/canvas.hpp>

#include "galapix/viewer.hpp"
#include "galapix/workspace.hpp"
#include "math/rect.hpp"

namespace galapix {

namespace {

/** zoom speed of ZoomIn / ZoomOut per second */
constexpr float kZoomSpeed = 4.0f;

/** pan speed per pixel of mouse motion */
constexpr float kPanSpeed = 4.0f;

Vector2f
to_world(Viewer& viewer, Vector2i const& pos)
{
  return viewer.get_state().screen2world(pos);
}

Rectf
rubber_band(Viewer& viewer, ToolDrag const& drag)
{
  return geom::normalize(Rectf(drag.press_world, to_world(viewer, drag.mouse_pos)));
}

float
screen_angle(Viewer& viewer, Vector2i const& pos)
{
  return atan2f(static_cast<float>(pos.y() - viewer.get_height() / 2),
                static_cast<float>(pos.x() - viewer.get_width() / 2));
}

float
selection_angle(Vector2f const& center, Vector2f const& world)
{
  return atan2f(center.y() - world.y(), center.x() - world.x());
}

} // namespace

void
tool_down(Viewer& viewer, ToolDrag& drag, Vector2i const& pos)
{
  drag.active = true;
  drag.mouse_pos = pos;
  drag.press_pos = pos;
  drag.press_world = to_world(viewer, pos);

  Workspace& workspace = *viewer.get_workspace();

  switch (drag.kind)
  {
    case ToolKind::Move:
      {
        // FIXME: add shift/ctrl modifier
        entt::entity const image = workspace.get_image(drag.press_world);
        if (image != entt::null) {
          if (workspace.is_selected(image)) {
            workspace.deselect(image);
          } else {
            workspace.select(image);
          }
          drag.select_box = false;
        } else {
          workspace.clear_selection();
          drag.select_box = true;
        }
      }
      break;

    case ToolKind::Resize:
      drag.center = workspace.get_selection_center();
      drag.applied_scale = 1.0f;
      break;

    case ToolKind::Rotate:
      drag.center = workspace.get_selection_center();
      drag.start_angle = selection_angle(drag.center, drag.press_world);
      break;

    case ToolKind::ViewRotate:
      drag.start_angle = screen_angle(viewer, pos);
      break;

    default:
      break;
  }
}

void
tool_move(Viewer& viewer, ToolDrag& drag, Vector2i const& pos, Vector2i const& rel)
{
  drag.mouse_pos = pos;

  switch (drag.kind)
  {
    case ToolKind::Pan:
      if (drag.active || viewer.get_trackball_mode()) {
        // FIXME: depending on rel leads to drift
        viewer.get_state().move(glm::vec2(rel.as_vec()) * kPanSpeed);
      }
      break;

    case ToolKind::Move:
      if (drag.active && !drag.select_box) {
        viewer.get_workspace()->move_selection(
          Vector2f(rel).as_vec() * (1.0f / viewer.get_state().get_scale()));
      }
      break;

    case ToolKind::Resize:
      if (drag.active) {
        Vector2f const p = to_world(viewer, pos);
        float const a = geom::distance(drag.center, p);
        float const b = geom::distance(drag.center, drag.press_world);
        if (b != 0.0f) {
          viewer.get_workspace()->scale_selection((1.0f / drag.applied_scale) * (a / b));
          drag.applied_scale = a / b;
        }
      }
      break;

    case ToolKind::Rotate:
      if (drag.active) {
        float const angle = selection_angle(drag.center, to_world(viewer, pos));
        std::cout << "Angle: " << ((drag.start_angle - angle) / glm::pi<float>() * 180.0f) << std::endl;
        viewer.get_workspace()->set_selection_angle(drag.start_angle - angle);
      }
      break;

    case ToolKind::ViewRotate:
      if (drag.active) {
        float const angle = screen_angle(viewer, pos);
        viewer.get_state().rotate((angle - drag.start_angle) / glm::pi<float>() * 180.0f);
        drag.start_angle = angle;
      }
      break;

    default:
      break;
  }
}

void
tool_up(Viewer& viewer, ToolDrag& drag, Vector2i const& pos)
{
  if (!drag.active) {
    return;
  }
  drag.active = false;
  drag.mouse_pos = pos;

  switch (drag.kind)
  {
    case ToolKind::ZoomRect:
      {
        Rectf const rect = rubber_band(viewer, drag);
        viewer.get_state().zoom_to(viewer.get_size(), rect);
        std::cout << "Zooming to: " << rect << std::endl;
      }
      break;

    case ToolKind::Grid:
      {
        Rectf const rect = rubber_band(viewer, drag);
        viewer.set_grid(Vector2f(rect.left(), rect.top()), rect.size());
      }
      break;

    case ToolKind::Move:
      if (drag.select_box) {
        Workspace& workspace = *viewer.get_workspace();
        workspace.select_images(workspace.get_images(rubber_band(viewer, drag)));
      }
      drag.select_box = false;
      break;

    default:
      break;
  }
}

void
tool_update(Viewer& viewer, ToolDrag& drag, Vector2i const& mouse_pos, float delta)
{
  if (!drag.active) {
    return;
  }

  float factor = 0.0f;
  switch (drag.kind)
  {
    case ToolKind::ZoomIn:  factor = 1.0f + kZoomSpeed * delta; break;
    case ToolKind::ZoomOut: factor = 1.0f / (1.0f + kZoomSpeed * delta); break;
    default: return;
  }

  if (viewer.get_trackball_mode()) {
    viewer.get_state().zoom(factor);
  } else {
    viewer.get_state().zoom(factor, mouse_pos);
  }
}

void
tool_draw(Viewer& viewer, ToolDrag const& drag, wstdisplay::Canvas& canvas)
{
  bool const rubber_band_visible =
    drag.active &&
    (drag.kind == ToolKind::ZoomRect || drag.kind == ToolKind::Grid ||
     (drag.kind == ToolKind::Move && drag.select_box));

  if (rubber_band_visible) {
    canvas.draw_rect(rubber_band(viewer, drag), surf::Color::from_rgb888(255, 255, 255),
                     1.0f / viewer.get_state().get_scale());
  }
}

bool
tool_animating(ToolDrag const& drag)
{
  return drag.active && (drag.kind == ToolKind::ZoomIn || drag.kind == ToolKind::ZoomOut);
}

} // namespace galapix

/* EOF */
