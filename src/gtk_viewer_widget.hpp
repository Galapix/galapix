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

#ifndef HEADER_GTK_VIEWER_WIDGET_HPP
#define HEADER_GTK_VIEWER_WIDGET_HPP

#include <gtkmm.h>
#include <libglademm/xml.h>
#include <glade/glade.h>
#include <iostream>
#include <gtkglmm.h>
#include <gtkmm/gl/widget.h>
#include <GL/gl.h>
#include <GL/glu.h>


class GtkViewerWidget 
  : public Gtk::DrawingArea,
    public Gtk::GL::Widget<GtkViewerWidget>
{
private:
public:
  GtkViewerWidget();
  virtual ~GtkViewerWidget();

  virtual void on_realize();
  virtual bool on_configure_event(GdkEventConfigure* event);
  virtual bool on_expose_event(GdkEventExpose* event);

private:
  GtkViewerWidget (const GtkViewerWidget&);
  GtkViewerWidget& operator= (const GtkViewerWidget&);
};

#endif

/* EOF */
