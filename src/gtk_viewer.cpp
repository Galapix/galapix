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
#include "gtk_viewer_widget.hpp"
#include "gtk_viewer.hpp"

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

  Glib::RefPtr<Gnome::Glade::Xml> xml = Gnome::Glade::Xml::create("src/galapix.glade");

  // start the event loop; exit when the specified window is closed
  Gtk::Window& window = dynamic_cast<Gtk::Window&>(*xml->get_widget("MainWindow"));

  Gtk::ScrolledWindow& hbox = dynamic_cast<Gtk::ScrolledWindow&>(*xml->get_widget("scrolledwindow1"));

  std::auto_ptr<Viewer> viewer = std::auto_ptr<Viewer>(new Viewer(workspace));

  viewer->set_grid(Vector2f(0,0), Sizef(16.0f, 16.0f));
  viewer->toggle_grid();
  viewer->toggle_pinned_grid();

  GtkViewerWidget viewer_widget(viewer.get());
  hbox.add(viewer_widget);
  viewer_widget.show();
      
  Gtk::Main::run(window);

  // Cleanup
  for(int i = 0; i < argc; ++i)
    free(argv[i]);
  free(argv);
}

/* EOF */
