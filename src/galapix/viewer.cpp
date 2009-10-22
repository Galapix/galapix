/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2008 Ingo Ruhnke <grumbel@gmx.de>
**
**  This program is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <SDL.h>
#include <fstream>
#include <boost/format.hpp>
#include <iostream>

#include "math/rgb.hpp"
#include "display/framebuffer.hpp"
#include "util/software_surface.hpp"
#include "math/vector2f.hpp"
#include "math/rect.hpp"
#include "math/rgba.hpp"
#include "galapix/workspace.hpp"
#include "tools/pan_tool.hpp"
#include "tools/move_tool.hpp"
#include "tools/zoom_tool.hpp"
#include "tools/zoom_rect_tool.hpp"
#include "tools/resize_tool.hpp"
#include "tools/rotate_tool.hpp"
#include "tools/grid_tool.hpp"
#include "tools/view_rotate_tool.hpp"
#include "util/filesystem.hpp"
#include "galapix/viewer.hpp"

Viewer* Viewer::current_ = 0;

Viewer::Viewer(Workspace* workspace_) :
  m_workspace(workspace_),
  m_mark_for_redraw(false),
  m_draw_grid(false),
  m_pin_grid(false),
  m_gamma(1.0f),
  m_brightness(0.0f),
  m_contrast(1.0f),
  m_state(),
  keyboard_zoom_in_tool(),
  keyboard_zoom_out_tool(),
  keyboard_view_rotate_tool(),
  pan_tool(),
  move_tool(),
  zoom_rect_tool(),
  zoom_in_tool(),
  zoom_out_tool(),
  resize_tool(),
  rotate_tool(),
  grid_tool(),
  left_tool(),
  middle_tool(),
  right_tool(),
  m_mouse_pos(),
  m_background_color(),
  m_background_colors(),
  m_grid_offset(0.0f, 0.0f),
  m_grid_size(400.0f, 300.0f),
  m_grid_color(255, 0, 0, 255)
{
  current_ = this;

  pan_tool       = boost::shared_ptr<PanTool>(new PanTool(this));
  move_tool      = boost::shared_ptr<MoveTool>(new MoveTool(this));
  zoom_rect_tool = boost::shared_ptr<ZoomRectTool>(new ZoomRectTool(this));
  resize_tool    = boost::shared_ptr<ResizeTool>(new ResizeTool(this));
  rotate_tool    = boost::shared_ptr<RotateTool>(new RotateTool(this));
  grid_tool      = boost::shared_ptr<GridTool>(new GridTool(this));

  zoom_in_tool  = boost::shared_ptr<ZoomTool>(new ZoomTool(this, -4.0f));
  zoom_out_tool = boost::shared_ptr<ZoomTool>(new ZoomTool(this,  4.0f));

  keyboard_zoom_in_tool  = boost::shared_ptr<ZoomTool>(new ZoomTool(this, -4.0f));
  keyboard_zoom_out_tool = boost::shared_ptr<ZoomTool>(new ZoomTool(this,  4.0f));

  keyboard_view_rotate_tool = boost::shared_ptr<ViewRotateTool>(new ViewRotateTool(this));

  left_tool   = zoom_in_tool.get();
  middle_tool = pan_tool.get();
  right_tool  = zoom_out_tool.get();

  m_background_color = 0;
  // Black to White
  m_background_colors.push_back(RGBA(  0,   0,   0));
  m_background_colors.push_back(RGBA( 64,  64,  64));
  m_background_colors.push_back(RGBA(128, 128, 128));
  m_background_colors.push_back(RGBA(255, 255, 255));
  // Rainbow
  m_background_colors.push_back(RGBA(255,   0,   0));
  m_background_colors.push_back(RGBA(255, 255,   0));
  m_background_colors.push_back(RGBA(255,   0, 255));
  m_background_colors.push_back(RGBA(  0, 255,   0));
  m_background_colors.push_back(RGBA(  0, 255, 255));
  m_background_colors.push_back(RGBA(  0,   0, 255));
  // Dimmed Rainbow
  m_background_colors.push_back(RGBA(128,   0,   0));
  m_background_colors.push_back(RGBA(128, 128,   0));
  m_background_colors.push_back(RGBA(128,   0, 128));
  m_background_colors.push_back(RGBA(  0, 128,   0));
  m_background_colors.push_back(RGBA(  0, 128, 128));
  m_background_colors.push_back(RGBA(  0,   0, 128));
}

void
Viewer::redraw()
{
  if (!m_mark_for_redraw)
  {
    m_mark_for_redraw = true;
  
#ifdef GALAPIX_SDL
    SDL_Event event;
    event.type = SDL_USEREVENT;
    event.user.code  = 1;
    event.user.data1 = 0;
    event.user.data2 = 0;
  
    while (SDL_PushEvent(&event) != 0) {}
#endif
  }
}

void
Viewer::draw()
{
  m_mark_for_redraw = false;
  Framebuffer::clear(m_background_colors[m_background_color]);

  bool clip_debug = false;

  glPushMatrix();

  if (clip_debug)
  {
    glTranslatef(static_cast<float>(Framebuffer::get_width())/2.0f, static_cast<float>(Framebuffer::get_height())/2.0f, 0.0f);
    glScalef(0.5f, 0.5f, 1.0f);
    glTranslatef(-static_cast<float>(Framebuffer::get_width())/2.0f, -static_cast<float>(Framebuffer::get_height())/2.0f, 0.0f);
  }

  Rectf cliprect = m_state.screen2world(Rect(0, 0, Framebuffer::get_width(), Framebuffer::get_height())); 

  if (m_state.get_angle() != 0.0f)
  {
    glTranslatef(static_cast<float>(Framebuffer::get_width())/2.0f, static_cast<float>(Framebuffer::get_height())/2.0f, 0.0f);
    glRotatef(m_state.get_angle(), 0.0f, 0.0f, 1.0f); // Rotates around 0.0
    glTranslatef(-static_cast<float>(Framebuffer::get_width())/2.0f, -static_cast<float>(Framebuffer::get_height())/2.0f, 0.0f);

    // FIXME: We enlarge the cliprect so much that we can rotate
    // freely, however this enlargement creates a cliprect that
    // might be quite a bit larger then what is really needed
    float  diagonal = cliprect.get_diagonal();
    Vector2f center = cliprect.get_center();
    cliprect.left   = center.x - diagonal;
    cliprect.right  = center.x + diagonal;
    cliprect.top    = center.y - diagonal;
    cliprect.bottom = center.y + diagonal;
  }

  glTranslatef(m_state.get_offset().x, m_state.get_offset().y, 0.0f);
  glScalef(m_state.get_scale(), m_state.get_scale(), 1.0f);

  if (clip_debug)
    Framebuffer::draw_rect(cliprect, RGB(255, 0, 255));
  
  m_workspace->draw(cliprect,
                  m_state.get_scale());

  left_tool->draw();
  middle_tool->draw();
  right_tool->draw();

  glPopMatrix();

  if (m_draw_grid)
  {
    if (m_pin_grid)
    {
      Framebuffer::draw_grid(m_grid_offset * m_state.get_scale() + m_state.get_offset(), 
                             m_grid_size * m_state.get_scale(),
                             m_grid_color);
    }
    else
    {
      Framebuffer::draw_grid(m_grid_offset, m_grid_size, m_grid_color);
    }
  }
}

void
Viewer::update(float delta)
{
  m_workspace->update(delta);

  zoom_in_tool ->update(m_mouse_pos, delta);
  zoom_out_tool->update(m_mouse_pos, delta);

  keyboard_zoom_in_tool ->update(m_mouse_pos, delta);
  keyboard_zoom_out_tool->update(m_mouse_pos, delta);
}

void
Viewer::on_mouse_motion(const Vector2i& pos, const Vector2i& rel)
{
  m_mouse_pos = pos;

  left_tool  ->move(m_mouse_pos, rel);
  middle_tool->move(m_mouse_pos, rel);
  right_tool ->move(m_mouse_pos, rel);

  keyboard_view_rotate_tool->move(m_mouse_pos, rel);
}

void
Viewer::on_mouse_button_down(const Vector2i& pos, int btn)
{
  m_mouse_pos = pos;

  switch(btn)
  {
    case SDL_BUTTON_LEFT:
      left_tool->down(pos);
      break;

    case SDL_BUTTON_RIGHT:
      right_tool->down(pos);
      break;

    case SDL_BUTTON_MIDDLE:
      middle_tool->down(pos);
      break;
  }
}

void
Viewer::on_mouse_button_up(const Vector2i& pos, int btn)
{
  m_mouse_pos = pos;

  switch(btn)
  {
    case SDL_BUTTON_LEFT:
      left_tool->up(pos);
      break;

    case SDL_BUTTON_RIGHT:
      right_tool->up(pos);
      break;

    case SDL_BUTTON_MIDDLE:
      middle_tool->up(pos);
      break;
  }
}

void
Viewer::on_key_up(int key)
{
  switch(key)
  {
    case SDLK_END:
      keyboard_zoom_out_tool->up(m_mouse_pos);
      break;

    case SDLK_HOME:
      keyboard_zoom_in_tool->up(m_mouse_pos);
      break;

    case SDLK_RSHIFT:
    case SDLK_LSHIFT:
      keyboard_view_rotate_tool->up(m_mouse_pos);
      break;
  }
}

void
Viewer::on_key_down(int key)
{
  switch(key)
  {
    case SDLK_END:
      keyboard_zoom_out_tool->down(m_mouse_pos);
      break;

    case SDLK_HOME:
      keyboard_zoom_in_tool->down(m_mouse_pos);
      break;

    case SDLK_RSHIFT:
    case SDLK_LSHIFT:
      keyboard_view_rotate_tool->down(m_mouse_pos);
      break;
  }
}

bool
Viewer::is_active() const
{
  return
    m_workspace->is_animated()   ||
    zoom_in_tool ->is_active() ||
    zoom_out_tool->is_active() ||
    keyboard_zoom_in_tool ->is_active() ||
    keyboard_zoom_out_tool->is_active();
}

void
Viewer::set_grid(const Vector2f& offset, const Sizef& size)
{
  if (m_pin_grid)
  {
    m_grid_offset = offset;
    m_grid_size   = size;
  }
  else
  {
    m_grid_offset = offset * m_state.get_scale() + m_state.get_offset();
    m_grid_size   = size * m_state.get_scale() ;
  }
}

void
Viewer::set_pan_tool()
{
  std::cout << "Pan&Zoom Tools selected" << std::endl;
  left_tool   = zoom_in_tool.get();
  right_tool  = zoom_out_tool.get();              
  middle_tool = pan_tool.get();
}

void
Viewer::set_zoom_tool()
{
  std::cout << "Zoom&Pan Tools selected" << std::endl;
  left_tool   = zoom_rect_tool.get();
  right_tool  = zoom_out_tool.get();
  middle_tool = pan_tool.get();
}

void
Viewer::set_grid_tool()
{
  std::cout << "Grid Tool selected" << std::endl;
  left_tool   = grid_tool.get();
  right_tool  = zoom_out_tool.get();
  middle_tool = pan_tool.get();
}

void
Viewer::set_move_resize_tool()
{
  std::cout << "Move&Resize Tools selected" << std::endl;
  left_tool   = move_tool.get();
  right_tool  = resize_tool.get();              
  middle_tool = pan_tool.get();
}

void
Viewer::set_move_rotate_tool()
{
  std::cout << "Move&Rotate Tools selected" << std::endl;
  left_tool   = move_tool.get();
  right_tool  = rotate_tool.get();
  middle_tool = pan_tool.get();
}

void
Viewer::increase_contrast()
{
  //contrast += 0.1f;
  m_contrast *= 1.1f;
  std::cout << "Contrast: " << m_contrast << std::endl;
  Framebuffer::apply_gamma_ramp(m_contrast, m_brightness, m_gamma);
}

void
Viewer::decrease_contrast()
{
  //contrast -= 0.1f;
  m_contrast /= 1.1f;
  std::cout << "Contrast: " << m_contrast << std::endl;
  Framebuffer::apply_gamma_ramp(m_contrast, m_brightness, m_gamma);
}

void
Viewer::increase_brightness()
{
  m_brightness += 0.03f;
  std::cout << "Brightness: " << m_brightness << std::endl;
  Framebuffer::apply_gamma_ramp(m_contrast, m_brightness, m_gamma);
}

void
Viewer::decrease_brightness()
{
  m_brightness -= 0.03f;
  std::cout << "Brightness: " << m_brightness << std::endl;
  Framebuffer::apply_gamma_ramp(m_contrast, m_brightness, m_gamma);
}

void
Viewer::increase_gamma()
{
  m_gamma *= 1.1f;
  std::cout << "Gamma: " << m_gamma << std::endl;
  Framebuffer::apply_gamma_ramp(m_contrast, m_brightness, m_gamma);
}

void
Viewer::decrease_gamma()
{
  m_gamma /= 1.1f;
  std::cout << "Gamma: " << m_gamma << std::endl;
  Framebuffer::apply_gamma_ramp(m_contrast, m_brightness, m_gamma);
}

void
Viewer::reset_gamma()
{
  m_brightness = 0.0f;
  m_contrast   = 1.0f;
  m_gamma      = 1.0f;
  Framebuffer::apply_gamma_ramp(m_contrast, m_brightness, m_gamma);
}

void
Viewer::toggle_grid()
{
  m_draw_grid = !m_draw_grid;
  std::cout << "Draw Grid: " << m_draw_grid << std::endl;
}

void
Viewer::layout_auto()
{
  m_workspace->layout_aspect(static_cast<float>(Framebuffer::get_width()),
                             static_cast<float>(Framebuffer::get_height()));
}

void
Viewer::layout_random()
{
  m_workspace->layout_random();
}

void
Viewer::layout_solve_overlaps()
{
  m_workspace->solve_overlaps();
}

void
Viewer::layout_spiral()
{
  m_workspace->layout_spiral();
}

void
Viewer::layout_tight()
{
  m_workspace->layout_tight(static_cast<float>(Framebuffer::get_width()),
                          static_cast<float>(Framebuffer::get_height()));
}

void
Viewer::layout_vertical()
{
  m_workspace->layout_vertical();
}

void
Viewer::finish_layout()
{
  m_workspace->finish_animation();
}

void
Viewer::toggle_pinned_grid()
{
  m_pin_grid = !m_pin_grid;
  std::cout << "Pin Grid: " << m_pin_grid << std::endl;
  if (!m_pin_grid)
  {
    m_grid_offset = m_grid_offset * m_state.get_scale() + m_state.get_offset();
    m_grid_size  *= m_state.get_scale();
  }
  else
  {
    m_grid_offset = (m_grid_offset - m_state.get_offset()) / m_state.get_scale();
    m_grid_size  /= m_state.get_scale();            
  }
}

void
Viewer::toggle_background_color(bool backwards)
{
  if (backwards)
  {
    m_background_color -= 1;
    if (m_background_color < 0)
      m_background_color = static_cast<int>(m_background_colors.size()) - 1;
  }
  else
  {
    m_background_color += 1;
    if (m_background_color >= static_cast<int>(m_background_colors.size()))
      m_background_color = 0;
  }
}

void
Viewer::zoom_home()
{
  m_state.set_offset(Vector2f(0.0f, 0.0f));
  m_state.set_angle(0.0f);
  m_state.set_scale(1.0f);
}

void
Viewer::zoom_to_selection()
{
  if (!m_workspace->get_selection().empty())
  {
    m_state.zoom_to(Framebuffer::get_size(),
                  m_workspace->get_selection().get_bounding_rect());
  }
  else
  {
    m_state.zoom_to(Framebuffer::get_size(),
                  m_workspace->get_bounding_rect());
  }
}

void 
Viewer::rotate_view_90()
{
  m_state.rotate(90.0f);
}

void 
Viewer::rotate_view_270()
{
  m_state.rotate(-90.0f);
}

void
Viewer::delete_selection()
{
  m_workspace->delete_selection();
}

void
Viewer::reset_view_rotation()
{
  m_state.set_angle(0.0f);
}

void
Viewer::toggle_trackball_mode()
{
  pan_tool->set_trackball_mode(!pan_tool->get_trackball_mode());
  if (pan_tool->get_trackball_mode())
  {
    std::cout << "Trackball mode active, press 't' to leave" << std::endl;
    SDL_ShowCursor(SDL_DISABLE);
    SDL_WM_GrabInput(SDL_GRAB_ON);
  }
  else
  {
    std::cout << "Trackball mode deactivated" << std::endl;
    SDL_ShowCursor(SDL_ENABLE);
    SDL_WM_GrabInput(SDL_GRAB_OFF);
  }
}

void
Viewer::load()
{
  m_workspace->load("/tmp/workspace-dump.galapix");
}

void
Viewer::save()
{
  std::ofstream out("/tmp/workspace-dump.galapix");
  m_workspace->save(out);
  out.close();
}

void
Viewer::refresh_selection()
{
  // FIXME: Make force on Shift-F5 and normal F5 only refresh if the file changed
  std::cout << "Refreshing tiles..." << std::endl;
  Selection selection = m_workspace->get_selection();
  bool force = true; // FIXME: keystates[SDLK_RSHIFT] || keystates[SDLK_LSHIFT];
  for(Selection::iterator i = selection.begin(); i != selection.end(); ++i)
  {
    (*i)->refresh(force);
  }
}

void
Viewer::clear_cache()
{
  std::cout << "Workspace: Clearing cache" << std::endl;
  m_workspace->clear_cache();
}

void
Viewer::cleanup_cache()
{
  std::cout << "Workspace: Cache Cleanup" << std::endl;
  m_workspace->cache_cleanup();
}

void
Viewer::sort_image_list()
{
  std::cout << "Workspace: Sorting" << std::endl;
  m_workspace->sort();
}

void
Viewer::shuffle_image_list()
{
  std::cout << "Workspace: Random Shuffle" << std::endl;
  m_workspace->random_shuffle();
}

void
Viewer::isolate_selection()
{
  m_workspace->isolate_selection();
}

void
Viewer::print_images()
{
  Rectf cliprect = m_state.screen2world(Rect(0, 0, Framebuffer::get_width(), Framebuffer::get_height()));
  m_workspace->print_images(cliprect);
}

void
Viewer::print_info()
{
  Rectf cliprect = m_state.screen2world(Rect(0, 0, Framebuffer::get_width(), Framebuffer::get_height()));
  m_workspace->print_info(cliprect);
}

void
Viewer::print_state()
{
  std::cout << m_state.get_offset() << " " << m_state.get_scale() << std::endl;
}

/* EOF */
