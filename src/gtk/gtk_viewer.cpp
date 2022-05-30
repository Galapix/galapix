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

#include <memory>
#include <string.h>
#include <gtkmm.h>
#include <gtkmm/builder.h>

#include "galapix/viewer.hpp"
#include "gtk/gtk_viewer_widget.hpp"
#include "gtk/gtk_viewer.hpp"

namespace galapix {

GtkViewer::GtkViewer(System& system) :
  m_system(system),
  workspace(),
  pan_tool_button(),
  zoom_tool_button(),
  grid_tool_button(),
  move_tool_button(),
  zoom_in_button(),
  zoom_out_button(),
  zoom_home_button(),
  grid_toggle(),
  grid_pin_toggle(),
  layout_regular_button(),
  layout_tight_button(),
  layout_random_button(),
  viewer()
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
  argv = static_cast<char**>(malloc(sizeof(char*) * 2));
  argv[0] = strdup("galapix");
  argv[1] = nullptr;

  Gtk::Main kit(&argc, &argv);

  Glib::RefPtr<Gtk::IconTheme> icon_theme = Gtk::IconTheme::get_default();
  icon_theme->append_search_path("data/icons/");

  Glib::RefPtr<Gtk::CssProvider> css_provider = Gtk::CssProvider::create();
  css_provider->load_from_path("data/gtk/style.css");

  Glib::RefPtr<Gtk::StyleContext> style_context = Gtk::StyleContext::create();
  Glib::RefPtr<Gdk::Screen> screen = Gdk::Screen::get_default();
  style_context->add_provider_for_screen(screen, css_provider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  Glib::RefPtr<Gtk::Builder> builder;

  builder = Gtk::Builder::create();
  builder->add_from_file("data/gtk/galapix.xml");

  // start the event loop; exit when the specified window is closed
  Gtk::Window* window = nullptr;
  builder->get_widget("MainWindow", window);

  Gtk::ScrolledWindow* hbox = nullptr;
  builder->get_widget("scrolledwindow1", hbox);

  viewer = std::make_unique<Viewer>(m_system, workspace);

  viewer->set_grid(Vector2f(0,0), Sizef(256.0f, 256.0f));
  //viewer->toggle_grid();
  //viewer->toggle_pinned_grid();

  GtkViewerWidget viewer_widget(viewer.get());
  hbox->add(viewer_widget);
  viewer_widget.show();

  // Toolbox
  builder->get_widget("PanToolButton", pan_tool_button);
  builder->get_widget("ZoomToolButton", zoom_tool_button);
  builder->get_widget("GridToolButton", grid_tool_button);
  builder->get_widget("MoveToolButton", move_tool_button);

  pan_tool_button ->signal_toggled().connect(sigc::mem_fun(this, &GtkViewer::on_pan_tool_toggled));
  zoom_tool_button->signal_toggled().connect(sigc::mem_fun(this, &GtkViewer::on_zoom_tool_toggled));
  grid_tool_button->signal_toggled().connect(sigc::mem_fun(this, &GtkViewer::on_grid_tool_toggled));
  move_tool_button->signal_toggled().connect(sigc::mem_fun(this, &GtkViewer::on_move_tool_toggled));

  // Toolbar
  builder->get_widget("zoom-in", zoom_in_button);
  builder->get_widget("zoom-out", zoom_out_button);
  builder->get_widget("zoom-home", zoom_home_button);

  zoom_in_button->signal_clicked().connect(sigc::mem_fun(this, &GtkViewer::on_zoom_in_clicked));
  zoom_out_button->signal_clicked().connect(sigc::mem_fun(this, &GtkViewer::on_zoom_out_clicked));
  zoom_home_button->signal_clicked().connect(sigc::mem_fun(this, &GtkViewer::on_zoom_home_clicked));

  builder->get_widget("grid-toggle", grid_toggle);
  builder->get_widget("grid-pin-toggle", grid_pin_toggle);

  grid_toggle->signal_toggled().connect(sigc::mem_fun(this, &GtkViewer::on_grid_toggle));
  grid_pin_toggle->signal_toggled().connect(sigc::mem_fun(this, &GtkViewer::on_grid_pin_toggle));

  builder->get_widget("layout-regular", layout_regular_button);
  builder->get_widget("layout-tight", layout_tight_button);
  builder->get_widget("layout-random", layout_random_button);

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
  Gtk::Main::run(*window);

  // Cleanup
  for(int i = 0; i < argc; ++i) {
    free(argv[i]);
  }
  free(argv);
}

void
GtkViewer::on_pan_tool_toggled()
{
  if (pan_tool_button->get_active()) {
    viewer->set_pan_tool();
  }
}

void
GtkViewer::on_zoom_tool_toggled()
{
  if (zoom_tool_button->get_active()) {
    viewer->set_zoom_tool();
  }
}

void
GtkViewer::on_grid_tool_toggled()
{
  if (grid_tool_button->get_active()) {
    viewer->set_grid_tool();
  }
}

void
GtkViewer::on_move_tool_toggled()
{
  if (move_tool_button->get_active()) {
    viewer->set_move_resize_tool();
  }
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

} // namespace galapix

/* EOF */
