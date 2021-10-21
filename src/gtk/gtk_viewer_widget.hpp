// Galapix - an image viewer for large image collections
// Copyright (C) 2008-2019 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef HEADER_GALAPIX_GTK_GTK_VIEWER_WIDGET_HPP
#define HEADER_GALAPIX_GTK_GTK_VIEWER_WIDGET_HPP

#include <iostream>
#include <gtkmm/glarea.h>

#include <wstdisplay/fwd.hpp>

class Viewer;

class GtkViewerWidget final : public Gtk::GLArea
{
public:
  GtkViewerWidget(Viewer* viewer);
  ~GtkViewerWidget() override;

  void on_realize() override;
  bool on_timeout();
  Glib::RefPtr<Gdk::GLContext> on_create_context() override;
  void on_resize(int width, int height) override;
  bool on_render(const Glib::RefPtr<Gdk::GLContext>& context) override;

  bool mouse_move(GdkEventMotion* event);
  bool mouse_down (GdkEventButton* event);
  bool mouse_up(GdkEventButton* event);
  bool scroll(GdkEventScroll* event);

  bool key_press(GdkEventKey* event);
  bool key_release(GdkEventKey* event);

private:
  Viewer* viewer;
  std::unique_ptr<wstdisplay::GraphicsContext> m_gc;
  Vector2i mouse_pos;

private:
  GtkViewerWidget (const GtkViewerWidget&);
  GtkViewerWidget& operator= (const GtkViewerWidget&);
};

#endif

/* EOF */
