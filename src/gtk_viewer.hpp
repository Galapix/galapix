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

#ifndef HEADER_GTK_VIEWER_HPP
#define HEADER_GTK_VIEWER_HPP

#include <gtkmm.h>

class Workspace;

class GtkViewer
{
private:
  Workspace* workspace;
  
  Gtk::RadioToolButton* pan_tool_button;
  Gtk::RadioToolButton* zoom_tool_button;
  Gtk::RadioToolButton* grid_tool_button;
  Gtk::RadioToolButton* move_tool_button;

  Gtk::ToolButton* zoom_in_button;
  Gtk::ToolButton* zoom_out_button;
  Gtk::ToolButton* zoom_home_button;

  Gtk::ToggleToolButton* grid_toggle;
  Gtk::ToggleToolButton* grid_pin_toggle;

  Gtk::RadioToolButton* layout_regular_button;
  Gtk::RadioToolButton* layout_tight_button;
  Gtk::RadioToolButton* layout_random_button;

  std::auto_ptr<Viewer> viewer;

public:
  GtkViewer();
  ~GtkViewer();

  void run();
  void set_workspace(Workspace* workspace_) { workspace = workspace_; }

  void on_pan_tool_toggled();
  void on_zoom_tool_toggled();
  void on_grid_tool_toggled();
  void on_move_tool_toggled();

  void on_grid_toggle();
  void on_grid_pin_toggle();

  void on_layout_toggle();

  void on_zoom_in_clicked();
  void on_zoom_out_clicked();
  void on_zoom_home_clicked();

private:
  GtkViewer (const GtkViewer&);
  GtkViewer& operator= (const GtkViewer&);
};

#endif

/* EOF */
