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

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include <gtkmm.h>

#include "framebuffer.hpp"
#include "viewer.hpp"
#include "gtk_viewer_widget.hpp"

GtkViewerWidget::GtkViewerWidget(Viewer* viewer_)
  : viewer(viewer_)
{
  Glib::RefPtr<Gdk::GL::Config> glconfig;

  glconfig = Gdk::GL::Config::create(Gdk::GL::MODE_RGB    |
                                     Gdk::GL::MODE_DEPTH  |
                                     Gdk::GL::MODE_DOUBLE);
  if (!glconfig)
    {
      std::cerr << "*** Cannot find the double-buffered visual.\n"
                << "*** Trying single-buffered visual.\n";

      // Try single-buffered visual
      glconfig = Gdk::GL::Config::create(Gdk::GL::MODE_RGB   |
                                         Gdk::GL::MODE_DEPTH);
      if (!glconfig)
        {
          throw std::runtime_error("*** Cannot find any OpenGL-capable visual.");
        }
    }
 
  set_gl_capability(glconfig);

  add_events(Gdk::POINTER_MOTION_MASK | Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK |
             Gdk::KEY_PRESS_MASK      | Gdk::KEY_RELEASE_MASK | 
             Gdk::ENTER_NOTIFY_MASK   | Gdk::LEAVE_NOTIFY_MASK);

  // Gdk::POINTER_MOTION_HINT_MASK |
  // Gdk::BUTTON_MOTION_MASK | Gdk::BUTTON1_MOTION_MASK | Gdk::BUTTON2_MOTION_MASK |
  // Gdk::BUTTON3_MOTION_MASK | 

  set_flags(get_flags()|Gtk::CAN_FOCUS);

  signal_button_release_event().connect(sigc::mem_fun(this, &GtkViewerWidget::mouse_up));
  signal_button_press_event().connect(sigc::mem_fun(this, &GtkViewerWidget::mouse_down));
  signal_motion_notify_event().connect(sigc::mem_fun(this, &GtkViewerWidget::mouse_move));

  signal_key_press_event().connect(sigc::mem_fun(this, &GtkViewerWidget::key_press));
  signal_key_release_event().connect(sigc::mem_fun(this, &GtkViewerWidget::key_release));


  Glib::signal_timeout().connect(sigc::mem_fun(this, &GtkViewerWidget::on_timeout),
                                 33);
}

GtkViewerWidget::~GtkViewerWidget()
{
}

bool
GtkViewerWidget::on_timeout()
{
  // FIXME: Calculate real time here
  viewer->update(0.033f);
  
  // FIXME: Troublesome
  queue_draw();
  
  return true;
}

void
GtkViewerWidget::on_realize()
{
  Gtk::DrawingArea::on_realize();

  Glib::RefPtr<Gdk::GL::Window> glwindow = get_gl_window();

  if (!glwindow->gl_begin(get_gl_context()))
    return;

  Framebuffer::init();
  Framebuffer::reshape(Size(get_width(), get_height()));

  glwindow->gl_end();
}

bool
GtkViewerWidget::on_configure_event(GdkEventConfigure* event)
{
  Glib::RefPtr<Gdk::GL::Window> glwindow = get_gl_window();

  // *** OpenGL BEGIN ***
  if (!glwindow->gl_begin(get_gl_context()))
    return false;

  Framebuffer::reshape(Size(get_width(), get_height()));

  glwindow->gl_end();
  // *** OpenGL END ***

  return true;
}

bool
GtkViewerWidget::on_expose_event(GdkEventExpose* event)
{
  Glib::RefPtr<Gdk::GL::Window> glwindow = get_gl_window();

  if (!glwindow->gl_begin(get_gl_context()))
    return false;

  viewer->draw();

  // Swap buffers.
  if (glwindow->is_double_buffered())
    glwindow->swap_buffers();
  else
    glFlush();

  glwindow->gl_end();

  return true;
}

bool
GtkViewerWidget::mouse_move(GdkEventMotion* event)
{
  //std::cout << "Motion: " << event->x << ", " << event->y << std::endl;
  Vector2i new_pos(event->x, event->y);
  viewer->on_mouse_motion(new_pos, new_pos - mouse_pos);
  mouse_pos = new_pos;

  // Trigger redraw
  queue_draw();

  return true;
}

bool
GtkViewerWidget::mouse_down(GdkEventButton* event)
{
  grab_focus();
  //std::cout << "Button Press: " << event->x << ", " << event->y << " - " << event->button << std::endl;
  viewer->on_mouse_button_down(Vector2i(event->x, event->y), event->button);
  return false;
}

bool
GtkViewerWidget::mouse_up(GdkEventButton* event)
{
  //std::cout << "Button Release: " << event->x << ", " << event->y << " - " << event->button << std::endl;
  viewer->on_mouse_button_up(Vector2i(event->x, event->y), event->button);
  return false;
}

bool
GtkViewerWidget::key_press(GdkEventKey* event)
{
  //std::cout << "KeyPress" << std::endl;
  std::cout << "v" << event->keyval << std::endl;
  switch(event->keyval)
    {
      case GDK_space:
        break;

      case GDK_l:
        viewer->print_state();
        break;

      case GDK_h:        
        viewer->zoom_home();
        break;

      case GDK_g:
        viewer->toggle_grid();
        break;

      case GDK_f:
        viewer->toggle_pinned_grid();
        break;

      default:
        break;
    }
  return true;
}

bool
GtkViewerWidget::key_release(GdkEventKey* event)
{ // /usr/include/gtk-2.0/gdk/gdkkeysyms.h
  std::cout << "KeyRelease" << std::endl;
  std::cout << "^" << event->keyval << std::endl;
  // event->hardware_keycode
  return true;
}

/* EOF */
