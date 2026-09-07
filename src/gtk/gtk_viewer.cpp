// Galapix - an image viewer for large image collections
// Copyright (C) 2008-2026 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include "gtk/gtk_viewer.hpp"

#include <cstdlib>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>

#include <gtkmm.h>
#include <gtkmm/builder.h>

#include "galapix/viewer.hpp"
#include "gtk/gtk_viewer_widget.hpp"

namespace galapix {
namespace {

/** Resolve UI/CSS/icon tree: GALAPIX_DATADIR, else ./data (dev), else share next to binary. */
std::filesystem::path
data_root()
{
  if (char const* env = std::getenv("GALAPIX_DATADIR"); env && *env) {
    return std::filesystem::path(env);
  }
  if (std::filesystem::is_directory("data/gtk")) {
    return std::filesystem::path("data");
  }
  // Installed layout: $prefix/share/galapix (binary in $prefix/bin)
  return std::filesystem::path("/usr/local/share/galapix");
}

} // namespace

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
  // Fake argc/argv for Gtk::Application (Galapix already parsed CLI).
  int argc = 1;
  char arg0[] = "galapix.gtk";
  char* argv_storage[] = { arg0, nullptr };
  char** argv = argv_storage;

  auto app = Gtk::Application::create(argc, argv, "org.galapix.gtk");

  std::filesystem::path const root = data_root();
  std::filesystem::path const ui = root / "gtk" / "galapix.xml";
  std::filesystem::path const css = root / "gtk" / "style.css";
  std::filesystem::path const icons = root / "icons";

  if (!std::filesystem::is_regular_file(ui)) {
    throw std::runtime_error(
      "GtkViewer: UI file not found: " + ui.string() +
      " (set GALAPIX_DATADIR to the directory containing gtk/ and icons/)");
  }

  if (std::filesystem::is_directory(icons)) {
    Glib::RefPtr<Gtk::IconTheme> icon_theme = Gtk::IconTheme::get_default();
    icon_theme->append_search_path(icons.string());
  }

  if (std::filesystem::is_regular_file(css)) {
    Glib::RefPtr<Gtk::CssProvider> css_provider = Gtk::CssProvider::create();
    css_provider->load_from_path(css.string());
    Glib::RefPtr<Gdk::Screen> screen = Gdk::Screen::get_default();
    Gtk::StyleContext::add_provider_for_screen(
      screen, css_provider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  }

  Glib::RefPtr<Gtk::Builder> builder = Gtk::Builder::create();
  builder->add_from_file(ui.string());

  Gtk::Window* window = nullptr;
  builder->get_widget("MainWindow", window);
  if (!window) {
    throw std::runtime_error("GtkViewer: MainWindow missing from UI file");
  }

  Gtk::ScrolledWindow* hbox = nullptr;
  builder->get_widget("scrolledwindow1", hbox);
  if (!hbox) {
    throw std::runtime_error("GtkViewer: scrolledwindow1 missing from UI file");
  }

  viewer = std::make_unique<Viewer>(m_system, workspace);
  viewer->set_grid(Vector2f(0, 0), Sizef(256.0f, 256.0f));

  GtkViewerWidget viewer_widget(viewer.get());
  hbox->add(viewer_widget);
  viewer_widget.show();

  builder->get_widget("PanToolButton", pan_tool_button);
  builder->get_widget("ZoomToolButton", zoom_tool_button);
  builder->get_widget("GridToolButton", grid_tool_button);
  builder->get_widget("MoveToolButton", move_tool_button);

  if (pan_tool_button) {
    pan_tool_button->signal_toggled().connect(
      sigc::mem_fun(*this, &GtkViewer::on_pan_tool_toggled));
  }
  if (zoom_tool_button) {
    zoom_tool_button->signal_toggled().connect(
      sigc::mem_fun(*this, &GtkViewer::on_zoom_tool_toggled));
  }
  if (grid_tool_button) {
    grid_tool_button->signal_toggled().connect(
      sigc::mem_fun(*this, &GtkViewer::on_grid_tool_toggled));
  }
  if (move_tool_button) {
    move_tool_button->signal_toggled().connect(
      sigc::mem_fun(*this, &GtkViewer::on_move_tool_toggled));
  }

  builder->get_widget("zoom-in", zoom_in_button);
  builder->get_widget("zoom-out", zoom_out_button);
  builder->get_widget("zoom-home", zoom_home_button);

  if (zoom_in_button) {
    zoom_in_button->signal_clicked().connect(
      sigc::mem_fun(*this, &GtkViewer::on_zoom_in_clicked));
  }
  if (zoom_out_button) {
    zoom_out_button->signal_clicked().connect(
      sigc::mem_fun(*this, &GtkViewer::on_zoom_out_clicked));
  }
  if (zoom_home_button) {
    zoom_home_button->signal_clicked().connect(
      sigc::mem_fun(*this, &GtkViewer::on_zoom_home_clicked));
  }

  builder->get_widget("grid-toggle", grid_toggle);
  builder->get_widget("grid-pin-toggle", grid_pin_toggle);

  if (grid_toggle) {
    grid_toggle->signal_toggled().connect(
      sigc::mem_fun(*this, &GtkViewer::on_grid_toggle));
  }
  if (grid_pin_toggle) {
    grid_pin_toggle->signal_toggled().connect(
      sigc::mem_fun(*this, &GtkViewer::on_grid_pin_toggle));
  }

  builder->get_widget("layout-regular", layout_regular_button);
  builder->get_widget("layout-tight", layout_tight_button);
  builder->get_widget("layout-random", layout_random_button);

  if (layout_regular_button) {
    layout_regular_button->signal_toggled().connect(
      sigc::mem_fun(*this, &GtkViewer::on_layout_toggle));
  }
  if (layout_tight_button) {
    layout_tight_button->signal_toggled().connect(
      sigc::mem_fun(*this, &GtkViewer::on_layout_toggle));
  }
  if (layout_random_button) {
    layout_random_button->signal_toggled().connect(
      sigc::mem_fun(*this, &GtkViewer::on_layout_toggle));
  }

  app->run(*window);
}

void
GtkViewer::on_pan_tool_toggled()
{
  if (pan_tool_button && pan_tool_button->get_active()) {
    viewer->set_pan_tool();
  }
}

void
GtkViewer::on_zoom_tool_toggled()
{
  if (zoom_tool_button && zoom_tool_button->get_active()) {
    viewer->set_zoom_tool();
  }
}

void
GtkViewer::on_grid_tool_toggled()
{
  if (grid_tool_button && grid_tool_button->get_active()) {
    viewer->set_grid_tool();
  }
}

void
GtkViewer::on_move_tool_toggled()
{
  if (move_tool_button && move_tool_button->get_active()) {
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
  if (layout_regular_button && layout_regular_button->get_active()) {
    viewer->layout_auto();
  } else if (layout_tight_button && layout_tight_button->get_active()) {
    viewer->layout_tight();
  } else if (layout_random_button && layout_random_button->get_active()) {
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
  viewer->get_state().zoom(1.0f / 1.5f);
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
