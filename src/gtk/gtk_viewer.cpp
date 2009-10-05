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

#include <memory>
#include <string.h>
#include <gtkmm.h>
#include <libglademm/xml.h>
#include <iostream>

#include "viewer.hpp"
#include "gtk/gtk_viewer_widget.hpp"
#include "gtk/gtk_viewer.hpp"

GtkViewer::GtkViewer()
{
}

GtkViewer::~GtkViewer()
{
}

void
GtkViewer::run()
{
  int    argc = 1;
  char** argv;

  // Fake command line argument construction
  argv = (char**)malloc(sizeof(char*) * 2);
  argv[0] = strdup("galapix");
  argv[1] = NULL;

  Gtk::Main kit(&argc, &argv);
  Gtk::GL::init(&argc, &argv);

  Glib::RefPtr<Gnome::Glade::Xml> xml = Gnome::Glade::Xml::create("data/glade/galapix.glade");

  // start the event loop; exit when the specified window is closed
  Gtk::Window& window = dynamic_cast<Gtk::Window&>(*xml->get_widget("MainWindow"));

  Gtk::ScrolledWindow& hbox = dynamic_cast<Gtk::ScrolledWindow&>(*xml->get_widget("scrolledwindow1"));

  viewer.reset(new Viewer(workspace));

  viewer->set_grid(Vector2f(0,0), Sizef(256.0f, 256.0f));
  //viewer->toggle_grid();
  //viewer->toggle_pinned_grid();

  GtkViewerWidget viewer_widget(viewer.get());
  hbox.add(viewer_widget);
  viewer_widget.show();

  // Toolbox
  pan_tool_button  = dynamic_cast<Gtk::RadioToolButton*>(xml->get_widget("PanToolButton"));
  zoom_tool_button = dynamic_cast<Gtk::RadioToolButton*>(xml->get_widget("ZoomToolButton"));
  grid_tool_button = dynamic_cast<Gtk::RadioToolButton*>(xml->get_widget("GridToolButton"));
  move_tool_button = dynamic_cast<Gtk::RadioToolButton*>(xml->get_widget("MoveToolButton"));
  
  pan_tool_button ->signal_toggled().connect(sigc::mem_fun(this, &GtkViewer::on_pan_tool_toggled));
  zoom_tool_button->signal_toggled().connect(sigc::mem_fun(this, &GtkViewer::on_zoom_tool_toggled));
  grid_tool_button->signal_toggled().connect(sigc::mem_fun(this, &GtkViewer::on_grid_tool_toggled));
  move_tool_button->signal_toggled().connect(sigc::mem_fun(this, &GtkViewer::on_move_tool_toggled));

  // Toolbar
  zoom_in_button   = dynamic_cast<Gtk::ToolButton*>(xml->get_widget("zoom-in"));
  zoom_out_button  = dynamic_cast<Gtk::ToolButton*>(xml->get_widget("zoom-out"));
  zoom_home_button = dynamic_cast<Gtk::ToolButton*>(xml->get_widget("zoom-home"));

  zoom_in_button->signal_clicked().connect(sigc::mem_fun(this, &GtkViewer::on_zoom_in_clicked));
  zoom_out_button->signal_clicked().connect(sigc::mem_fun(this, &GtkViewer::on_zoom_out_clicked));
  zoom_home_button->signal_clicked().connect(sigc::mem_fun(this, &GtkViewer::on_zoom_home_clicked));

  grid_toggle     = dynamic_cast<Gtk::ToggleToolButton*>(xml->get_widget("grid-toggle"));
  grid_pin_toggle = dynamic_cast<Gtk::ToggleToolButton*>(xml->get_widget("grid-pin-toggle"));

  grid_toggle->signal_toggled().connect(sigc::mem_fun(this, &GtkViewer::on_grid_toggle));
  grid_pin_toggle->signal_toggled().connect(sigc::mem_fun(this, &GtkViewer::on_grid_pin_toggle));
  
  layout_regular_button = dynamic_cast<Gtk::RadioToolButton*>(xml->get_widget("layout-regular"));
  layout_tight_button   = dynamic_cast<Gtk::RadioToolButton*>(xml->get_widget("layout-tight"));
  layout_random_button  = dynamic_cast<Gtk::RadioToolButton*>(xml->get_widget("layout-random"));

  layout_regular_button->signal_toggled().connect(sigc::mem_fun(this, &GtkViewer::on_layout_toggle));
  layout_tight_button  ->signal_toggled().connect(sigc::mem_fun(this, &GtkViewer::on_layout_toggle));
  layout_random_button ->signal_toggled().connect(sigc::mem_fun(this, &GtkViewer::on_layout_toggle));

  /*
  // Action Group stuff
  //  * http://www.gtkmm.org/docs/gtkmm-2.4/docs/tutorial/html/sec-printing-example.html
  //  *  http://www.gtkmm.org/docs/gtkmm-2.4/docs/tutorial/html/ch11.html
  Glib::RefPtr<Gtk::ActionGroup> action_group;
  action_group = Gtk::ActionGroup::create();
  
  action_group->add(Gtk::Action::create("FileMenu", "_File"));

  action_group->add(Gtk::Action::create("New", Gtk::Stock::NEW),
                    sigc::mem_fun(*this, &GtkViewer::on_menu_file_new));

  Glib::RefPtr<Gtk::UIManager> ui_manager = Gtk::UIManager::create();
  ui_manager->insert_action_group(action_group);

  Glib::ustring ui_info = 
    "<ui>"
    "  <menubar name='MenuBar'>"
    "    <menu action='FileMenu'>"
    "      <menuitem action='New'/>"
    "    </menu>"
    "  </menubar>"
    "</ui>";

  ui_manager->add_ui_from_string(ui_info);

  // Gtk::Widget* menubar = ui_manager->get_widget("/MenuBar");
  // if (menubar)
  //    vbox.pack_start(*menubar, Gtk::PACK_SHRINK);

  // Gtk::Widget* toolbar = ui_manager->get_widget("/ToolBar") ;
  // if (toolbar)
  //    vbox.pack_start(*pToolbar, Gtk::PACK_SHRINK);
  */

  // Run the thing
  Gtk::Main::run(window);

  // Cleanup
  for(int i = 0; i < argc; ++i)
    free(argv[i]);
  free(argv);
}

void
GtkViewer::on_pan_tool_toggled()
{
  if (pan_tool_button->get_active())
    viewer->set_pan_tool();
}

void
GtkViewer::on_zoom_tool_toggled()
{
  if (zoom_tool_button->get_active())
    viewer->set_zoom_tool();
}

void
GtkViewer::on_grid_tool_toggled()
{
  if (grid_tool_button->get_active())
    viewer->set_grid_tool();
}

void
GtkViewer::on_move_tool_toggled()
{
  if (move_tool_button->get_active())
    viewer->set_move_resize_tool();
}

void
GtkViewer::on_grid_toggle()
{
  viewer->toggle_grid();
}

void
GtkViewer::on_grid_pin_toggle()
{
  viewer->toggle_pinned_grid();
}

void
GtkViewer::on_layout_toggle()
{
  if (layout_regular_button->get_active())
    {
      viewer->layout_auto();
    }
  else if (layout_tight_button->get_active())
    {
      viewer->layout_tight();
    }
  else if (layout_random_button->get_active())
    {
      viewer->layout_random();
    }
}

void
GtkViewer::on_zoom_in_clicked()
{
  viewer->get_state().zoom(1.5f);
}

void
GtkViewer::on_zoom_out_clicked()
{
  viewer->get_state().zoom(1.0f/1.5f);
}

void
GtkViewer::on_zoom_home_clicked()
{
  viewer->zoom_to_selection(); 
}

void
GtkViewer::on_menu_file_new()
{
  
}

/* EOF */
