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

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <logmich/log.hpp>

#include <glm/gtx/io.hpp>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <surf/software_surface.hpp>
#include <surf/color.hpp>
#include <wstdisplay/graphics_context.hpp>

#include "galapix/system.hpp"
#include "galapix/viewer.hpp"

#ifdef HAVE_THUMTOO
#  include "thumtoo/thumtoo_callback_queue.hpp"
#  include "thumtoo/thumtoo_tile_provider.hpp"
#  include "thumtoo/thumtoo_uri.hpp"
#  include <thumtoo/archive.hpp>
#  include <thumtoo/client.hpp>
#  include <thumtoo/image.hpp>
#  include <thumtoo/pdf.hpp>
#  include <filesystem>
#endif
#include "galapix/image.hpp"
#include "util/url.hpp"
#include "galapix/image_tile_cache.hpp"
#include "galapix/workspace.hpp"
#include "math/rect.hpp"
#include "math/vector2f.hpp"
#include "tools/grid_tool.hpp"
#include "tools/move_tool.hpp"
#include "tools/pan_tool.hpp"
#include "tools/resize_tool.hpp"
#include "tools/rotate_tool.hpp"
#include "tools/view_rotate_tool.hpp"
#include "tools/zoom_rect_tool.hpp"
#include "tools/zoom_tool.hpp"
#include "util/filesystem.hpp"

namespace galapix {

using namespace surf;

Viewer* Viewer::current_ = nullptr;

Viewer::Viewer(System& system, Workspace* workspace_) :
  m_system(system),
  m_workspace(workspace_),
  m_job_manager(/*threads=*/2),
  m_mark_for_redraw(false),
  m_draw_grid(false),
  m_pin_grid(false),
  m_gamma(1.0f),
  m_brightness(0.0f),
  m_contrast(1.0f),
  m_state(*this),
  m_size(),
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
  m_job_manager.start_thread();
  current_ = this;

  pan_tool       = std::make_unique<PanTool>(this);
  move_tool      = std::make_unique<MoveTool>(this);
  zoom_rect_tool = std::make_unique<ZoomRectTool>(this);
  resize_tool    = std::make_unique<ResizeTool>(this);
  rotate_tool    = std::make_unique<RotateTool>(this);
  grid_tool      = std::make_unique<GridTool>(this);

  zoom_in_tool  = std::make_unique<ZoomTool>(this, -4.0f);
  zoom_out_tool = std::make_unique<ZoomTool>(this,  4.0f);

  keyboard_zoom_in_tool  = std::make_unique<ZoomTool>(this, -4.0f);
  keyboard_zoom_out_tool = std::make_unique<ZoomTool>(this,  4.0f);

  keyboard_view_rotate_tool = std::make_unique<ViewRotateTool>(this);

  left_tool   = zoom_in_tool.get();
  middle_tool = pan_tool.get();
  right_tool  = zoom_out_tool.get();

  m_background_color = 0;
  // Black to White
  m_background_colors.push_back(surf::Color::from_rgb888(  0,   0,   0));
  m_background_colors.push_back(surf::Color::from_rgb888( 64,  64,  64));
  m_background_colors.push_back(surf::Color::from_rgb888(128, 128, 128));
  m_background_colors.push_back(surf::Color::from_rgb888(255, 255, 255));
  // Rainbow
  m_background_colors.push_back(surf::Color::from_rgb888(255,   0,   0));
  m_background_colors.push_back(surf::Color::from_rgb888(255, 255,   0));
  m_background_colors.push_back(surf::Color::from_rgb888(255,   0, 255));
  m_background_colors.push_back(surf::Color::from_rgb888(  0, 255,   0));
  m_background_colors.push_back(surf::Color::from_rgb888(  0, 255, 255));
  m_background_colors.push_back(surf::Color::from_rgb888(  0,   0, 255));
  // Dimmed Rainbow
  m_background_colors.push_back(surf::Color::from_rgb888(128,   0,   0));
  m_background_colors.push_back(surf::Color::from_rgb888(128, 128,   0));
  m_background_colors.push_back(surf::Color::from_rgb888(128,   0, 128));
  m_background_colors.push_back(surf::Color::from_rgb888(  0, 128,   0));
  m_background_colors.push_back(surf::Color::from_rgb888(  0, 128, 128));
  m_background_colors.push_back(surf::Color::from_rgb888(  0,   0, 128));
}

Viewer::~Viewer()
{
  // Same sequence as ViewerCommand: signal quit, then join. Thread::~Thread
  // asserts m_state == kJoined — stop_thread alone left workers running.
  try {
    m_job_manager.abort_thread();
    m_job_manager.join_thread();
  } catch (...) {
  }
  if (current_ == this) {
    current_ = nullptr;
  }
}

void
Viewer::redraw()
{
  // Workers may call redraw(); coalesce with atomic CAS.
  bool expected = false;
  if (m_mark_for_redraw.compare_exchange_strong(expected, true))
  {
    m_system.trigger_redraw();
  }
}

void
Viewer::draw(wstdisplay::GraphicsContext& gc)
{
#ifdef HAVE_THUMTOO
  ThumtooCallbackQueue::instance().pump();
#endif
  m_mark_for_redraw.store(false);

  // Stagger new provider jobs across frames (see ImageTileCache budget).
  ImageTileCache::begin_frame_request_budget(128);

  bool clip_debug = false;

  glm::mat4 modelview = glm::mat4(1);

  if (clip_debug)
  {
    modelview *= glm::translate(glm::vec3(static_cast<float>(m_size.width())/2.0f,
                                          static_cast<float>(m_size.height())/2.0f,
                                          0.0f));
    modelview *= glm::scale(glm::vec3(0.5f, 0.5f, 1.0f));
    modelview *= glm::translate(glm::vec3(-static_cast<float>(m_size.width())/2.0f,
                                          -static_cast<float>(m_size.height())/2.0f,
                                          0.0f));
  }

  Rectf cliprect = m_state.screen2world(Rect(0, 0, m_size.width(), m_size.height()));

  if (m_state.get_angle() != 0.0f)
  {
    modelview *= glm::translate(glm::vec3(static_cast<float>(m_size.width())/2.0f,
                                          static_cast<float>(m_size.height())/2.0f,
                                          0.0f));
    modelview *= glm::rotate(glm::radians(m_state.get_angle()), glm::vec3(0.0f, 0.0f, 1.0f)); // Rotates around 0.0
    modelview *= glm::translate(glm::vec3(-static_cast<float>(m_size.width())/2.0f,
                                          -static_cast<float>(m_size.height())/2.0f,
                                          0.0f));

    // FIXME: We enlarge the cliprect so much that we can rotate
    // freely, however this enlargement creates a cliprect that
    // might be quite a bit larger then what is really needed
    float  diagonal = geom::diagonal(cliprect);
    Vector2f center = geom::center(cliprect).as_vec();

    cliprect = Rectf(center.x() - diagonal,
                     center.y() - diagonal,
                     center.x() + diagonal,
                     center.y() + diagonal);
  }

  modelview *= glm::translate(glm::vec3(m_state.get_offset().x(), m_state.get_offset().y(), 0.0f));
  modelview *= glm::scale(glm::vec3(m_state.get_scale(), m_state.get_scale(), 1.0f));

  gc.set_modelview(modelview);
  gc.clear(m_background_colors[static_cast<size_t>(m_background_color)]);

  if (clip_debug)
  {
    gc.draw_rect(cliprect, surf::Color::from_rgb888(255, 0, 255));
  }

  m_workspace->prepare_tiles(cliprect, m_state.get_scale());
  m_workspace->draw(gc, cliprect, m_state.get_scale());

  left_tool->draw(gc);
  middle_tool->draw(gc);
  right_tool->draw(gc);

  gc.set_modelview(glm::mat4(1));
  if (m_draw_grid)
  {
    if (m_pin_grid)
    {
      gc.draw_grid(m_grid_offset.as_vec() * m_state.get_scale() + m_state.get_offset().as_vec(),
                             m_grid_size * m_state.get_scale(),
                             m_grid_color);
    }
    else
    {
      gc.draw_grid(m_grid_offset, m_grid_size, m_grid_color);
    }
  }

  // First-paint / fill progress when GALAPIX_OPEN_TIMING is set. Pre-viewer
  // open is already timed in ViewerCommand; multi-second delays are here.
  if (std::getenv("GALAPIX_OPEN_TIMING") != nullptr && m_workspace) {
    using clock = std::chrono::steady_clock;
    static clock::time_point t0{};
    static int frame = 0;
    static bool inited = false;
    static bool saw_requests = false;
    static bool logged_first_ready = false;
    static bool logged_idle = false;

    if (!inited) {
      t0 = clock::now();
      inited = true;
      std::cout << "[open-timing] viewer first draw\n";
    }
    ++frame;

    int requests = 0, uploads = 0, entries = 0, ready = 0;
    m_workspace->tile_load_stats(requests, uploads, entries, &ready);
    double sec = std::chrono::duration<double>(clock::now() - t0).count();

    if (frame <= 10 || (frame <= 120 && frame % 15 == 0) || frame % 60 == 0) {
      std::cout << "[open-timing] frame " << frame
                << " t=" << sec << "s"
                << " req=" << requests
                << " upload_q=" << uploads
                << " ready=" << ready
                << " cache=" << entries << "\n";
      if (requests > 0) {
        std::cout << "[open-timing] stuck REQUESTED dump:\n";
        m_workspace->dump_stuck_tile_requests(8);
      }
    }
    if (!logged_first_ready && ready > 0) {
      logged_first_ready = true;
      std::cout << "[open-timing] first_ready_surface t=" << sec << "s"
                << " ready=" << ready << "\n";
    }
    if (requests > 0) {
      saw_requests = true;
    }
    if (saw_requests && !logged_idle && requests == 0 && uploads == 0) {
      logged_idle = true;
      std::cout << "[open-timing] pending_idle t=" << sec << "s"
                << " ready=" << ready
                << " cache=" << entries
                << " (no outstanding tile requests/uploads)\n";
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
Viewer::on_mouse_motion(Vector2i const& pos, Vector2i const& rel)
{
  m_mouse_pos = pos;

  left_tool  ->move(m_mouse_pos, rel);
  middle_tool->move(m_mouse_pos, rel);
  right_tool ->move(m_mouse_pos, rel);

  keyboard_view_rotate_tool->move(m_mouse_pos, rel);
}

void
Viewer::on_mouse_button_down(Vector2i const& pos, MouseButton btn)
{
  m_mouse_pos = pos;

  switch(btn)
  {
    case MouseButton::LEFT:
      left_tool->down(pos);
      break;

    case MouseButton::RIGHT:
      right_tool->down(pos);
      break;

    case MouseButton::MIDDLE:
      middle_tool->down(pos);
      break;
  }
}

void
Viewer::on_mouse_button_up(Vector2i const& pos, MouseButton btn)
{
  m_mouse_pos = pos;

  switch(btn)
  {
    case MouseButton::LEFT:
      left_tool->up(pos);
      break;

    case MouseButton::RIGHT:
      right_tool->up(pos);
      break;

    case MouseButton::MIDDLE:
      middle_tool->up(pos);
      break;
  }
}

void
Viewer::on_key_up(Key key)
{
  switch(key)
  {
    case Key::ZOOM_OUT:
      keyboard_zoom_out_tool->up(m_mouse_pos);
      break;

    case Key::ZOOM_IN:
      keyboard_zoom_in_tool->up(m_mouse_pos);
      break;

    case Key::ROTATE:
      keyboard_view_rotate_tool->up(m_mouse_pos);
      break;

    default:
      break;
  }
}

void
Viewer::on_key_down(Key key)
{
  switch(key)
  {
    case Key::ZOOM_OUT:
      keyboard_zoom_out_tool->down(m_mouse_pos);
      break;

    case Key::ZOOM_IN:
      keyboard_zoom_in_tool->down(m_mouse_pos);
      break;

    case Key::ROTATE:
      keyboard_view_rotate_tool->down(m_mouse_pos);
      break;

    default:
      break;
  }
}

bool
Viewer::is_active() const
{
  return
    zoom_in_tool ->is_active() ||
    zoom_out_tool->is_active() ||
    keyboard_zoom_in_tool ->is_active() ||
    keyboard_zoom_out_tool->is_active();
}

void
Viewer::set_grid(Vector2f const& offset, Sizef const& size)
{
  if (m_pin_grid)
  {
    m_grid_offset = offset;
    m_grid_size   = size;
  }
  else
  {
    m_grid_offset = offset.as_vec() * m_state.get_scale() + m_state.get_offset().as_vec();
    m_grid_size   = size * m_state.get_scale() ;
  }
}

void
Viewer::set_pan_tool()
{
  log_info("Pan&Zoom Tools selected");
  left_tool   = zoom_in_tool.get();
  right_tool  = zoom_out_tool.get();
  middle_tool = pan_tool.get();
}

void
Viewer::set_zoom_tool()
{
  log_info("Zoom&Pan Tools selected");
  left_tool   = zoom_rect_tool.get();
  right_tool  = zoom_out_tool.get();
  middle_tool = pan_tool.get();
}

void
Viewer::set_grid_tool()
{
  log_info("Grid Tool selected");
  left_tool   = grid_tool.get();
  right_tool  = zoom_out_tool.get();
  middle_tool = pan_tool.get();
}

void
Viewer::set_move_resize_tool()
{
  log_info("Move&Resize Tools selected");
  left_tool   = move_tool.get();
  right_tool  = resize_tool.get();
  middle_tool = pan_tool.get();
}

void
Viewer::set_move_rotate_tool()
{
  log_info("Move&Rotate Tools selected");
  left_tool   = move_tool.get();
  right_tool  = rotate_tool.get();
  middle_tool = pan_tool.get();
}

void
Viewer::increase_contrast()
{
  //contrast += 0.1f;
  m_contrast *= 1.1f;
  log_info("Contrast: {}", m_contrast);
  //Framebuffer::apply_gamma_ramp(m_contrast, m_brightness, m_gamma);
}

void
Viewer::decrease_contrast()
{
  //contrast -= 0.1f;
  m_contrast /= 1.1f;
  log_info("Contrast: {}", m_contrast);
  //Framebuffer::apply_gamma_ramp(m_contrast, m_brightness, m_gamma);
}

void
Viewer::increase_brightness()
{
  m_brightness += 0.03f;
  log_info("Brightness: {}", m_brightness);
  //Framebuffer::apply_gamma_ramp(m_contrast, m_brightness, m_gamma);
}

void
Viewer::decrease_brightness()
{
  m_brightness -= 0.03f;
  log_info("Brightness: {}", m_brightness);
  //Framebuffer::apply_gamma_ramp(m_contrast, m_brightness, m_gamma);
}

void
Viewer::increase_gamma()
{
  m_gamma *= 1.1f;
  log_info("Gamma: {}", m_gamma);
  //Framebuffer::apply_gamma_ramp(m_contrast, m_brightness, m_gamma);
}

void
Viewer::decrease_gamma()
{
  m_gamma /= 1.1f;
  log_info("Gamma: {}", m_gamma);
  //Framebuffer::apply_gamma_ramp(m_contrast, m_brightness, m_gamma);
}

void
Viewer::reset_gamma()
{
  m_brightness = 0.0f;
  m_contrast   = 1.0f;
  m_gamma      = 1.0f;
  //Framebuffer::apply_gamma_ramp(m_contrast, m_brightness, m_gamma);
}

void
Viewer::toggle_grid()
{
  m_draw_grid = !m_draw_grid;
  log_info("Draw Grid: {}", m_draw_grid);
}

void
Viewer::layout_auto()
{
  m_workspace->layout_aspect(static_cast<float>(m_size.width()),
                             static_cast<float>(m_size.height()));
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
  m_workspace->layout_tight(static_cast<float>(m_size.width()),
                            static_cast<float>(m_size.height()));
}

void
Viewer::layout_vertical()
{
  m_workspace->layout_vertical();
}

void
Viewer::toggle_pinned_grid()
{
  m_pin_grid = !m_pin_grid;
  log_info("Pin Grid: {}", m_pin_grid);
  if (!m_pin_grid)
  {
    m_grid_offset = m_grid_offset.as_vec() * m_state.get_scale() + m_state.get_offset().as_vec();
    m_grid_size   = m_grid_size * m_state.get_scale();
  }
  else
  {
    m_grid_offset = (m_grid_offset.as_vec() - m_state.get_offset().as_vec()) / m_state.get_scale();
    m_grid_size   = m_grid_size / m_state.get_scale();
  }
}

void
Viewer::toggle_background_color(bool backwards)
{
  if (backwards)
  {
    m_background_color -= 1;
    if (m_background_color < 0) {
      m_background_color = static_cast<int>(m_background_colors.size()) - 1;
    }
  }
  else
  {
    m_background_color += 1;
    if (m_background_color >= static_cast<int>(m_background_colors.size())) {
      m_background_color = 0;
    }
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
  if (!m_workspace->get_selection()->empty())
  {
    m_state.zoom_to(m_size, m_workspace->get_selection()->get_bounding_rect());
  }
  else
  {
    m_state.zoom_to(m_size, m_workspace->get_bounding_rect());
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
    log_info("Trackball mode active, press 't' to leave");
    m_system.set_trackball_mode(true);
  }
  else
  {
    log_info("Trackball mode deactivated");
    m_system.set_trackball_mode(false);
  }
}

bool
Viewer::get_trackball_mode() const
{
  return pan_tool->get_trackball_mode();
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
  log_info("Viewer::refresh_selection()");
  SelectionPtr selection = m_workspace->get_selection();
  bool force = true; // FIXME: keystates[SDLK_RSHIFT] || keystates[SDLK_LSHIFT];
  for(Selection::iterator i = selection->begin(); i != selection->end(); ++i)
  {
    (*i)->refresh(force);
  }
}

void
Viewer::clear_cache()
{
  log_info("Workspace: Clearing cache");
  m_workspace->clear_cache();
}

void
Viewer::cleanup_cache()
{
  log_info("Workspace: Cache Cleanup");
  m_workspace->cache_cleanup();
}

void
Viewer::sort_reverse_image_list()
{
  log_info("Workspace: Reverse Sorting");
  m_workspace->sort_reverse();
}

void
Viewer::sort_image_list()
{
  log_info("Workspace: Sorting");
  m_workspace->sort();
}

void
Viewer::shuffle_image_list()
{
  log_info("Workspace: Random Shuffle");
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
  Rectf cliprect = m_state.screen2world(Rect(m_size));
  m_workspace->print_images(cliprect);
}

void
Viewer::print_info()
{
  Rectf cliprect = m_state.screen2world(Rect(m_size));
  m_workspace->print_info(cliprect);
}

void
Viewer::print_state()
{
  // User-facing status (key "l"): use stdout like print_images/print_info.
  // log_info is silent unless --verbose / --debug.
  std::cout << "-- Viewer state -----------------------------------------" << std::endl;
  std::cout << "  offset: (" << m_state.get_offset().x() << ", "
            << m_state.get_offset().y() << ")  scale: "
            << m_state.get_scale() << std::endl;
  if (m_workspace) {
    int requests = 0;
    int uploads = 0;
    int cache_entries = 0;
    m_workspace->tile_load_stats(requests, uploads, cache_entries);
    std::cout << "  tiles: pending_requests=" << requests
              << "  pending_uploads=" << uploads
              << "  cache_entries=" << cache_entries << std::endl;
    int ov_idle = 0, ov_loading = 0, ov_ready = 0, ov_failed = 0;
    m_workspace->overview_stats(ov_idle, ov_loading, ov_ready, ov_failed);
    std::cout << "  overview: idle=" << ov_idle
              << "  loading=" << ov_loading
              << "  ready=" << ov_ready
              << "  failed=" << ov_failed
              << "  (stdio files only; archives skip)" << std::endl;
  } else {
    std::cout << "  workspace: (none)" << std::endl;
  }
#if defined(HAVE_THUMTOO)
  std::cout << "  thumtoo callback queue: "
            << ThumtooCallbackQueue::instance().size() << std::endl;
#endif
  std::cout << "--------------------------------------------------------" << std::endl;
}

void
Viewer::reshape(Size const& size)
{
  m_size = size;
}


void
Viewer::open_paths(std::vector<std::string> const& paths)
{
  if (!m_workspace || paths.empty()) {
    return;
  }

  std::vector<URL> urls;
  for (std::string const& raw : paths) {
    if (raw.empty()) {
      continue;
    }
    std::string path = raw;
    // SDL may give file:// URIs on some platforms.
    if (path.rfind("file://", 0) == 0) {
      path = path.substr(7);
      // file:///path → /path
      if (path.size() >= 2 && path[0] == '/' && path[1] == '/') {
        path = path.substr(1);
      }
    }
    try {
      if (std::filesystem::is_directory(path)) {
        Filesystem::generate_image_file_list(path, urls);
      } else if (std::filesystem::is_regular_file(path) || URL::is_url(raw)) {
        if (URL::is_url(raw) && raw.rfind("file://", 0) != 0) {
          urls.push_back(URL::from_string(raw));
        } else {
          urls.push_back(URL::from_filename(path));
        }
      } else {
        log_info("open_paths: skip missing path {}", path);
      }
    } catch (std::exception const& err) {
      log_info("open_paths: {}: {}", path, err.what());
    }
  }

  if (urls.empty()) {
    return;
  }

#ifdef HAVE_THUMTOO
  if (!m_thumtoo) {
    thumtoo::image_library_init();
    std::filesystem::path cache;
    if (char const* xdg = std::getenv("XDG_CACHE_HOME"); xdg && *xdg) {
      cache = std::filesystem::path(xdg) / "thumtoo";
    } else if (char const* home = std::getenv("HOME"); home && *home) {
      cache = std::filesystem::path(home) / ".cache" / "thumtoo";
    } else {
      cache = std::filesystem::path(".cache") / "thumtoo";
    }
    m_thumtoo = thumtoo::Client::open(
      cache, ThumtooCallbackQueue::instance().make_executor());
  }

  std::vector<URL> expanded;
  for (URL const& u : urls) {
    if (!u.has_stdio_name()) {
      expanded.push_back(u);
      continue;
    }
    std::filesystem::path const abspath =
      std::filesystem::absolute(u.get_stdio_name());
    if (!std::filesystem::is_regular_file(abspath)) {
      expanded.push_back(u);
      continue;
    }
    if (thumtoo::is_likely_pdf_path(abspath)) {
      auto pages = thumtoo::pdf_page_count(abspath);
      if (pages && *pages > 0) {
        int const n = std::min(*pages, 512);
        for (int page = 1; page <= n; ++page) {
          expanded.push_back(URL::from_string(
            "file://" + abspath.string() + "//page:" + std::to_string(page)));
        }
        continue;
      }
    }
    if (thumtoo::is_likely_archive_path(abspath)) {
      auto toc = thumtoo::read_archive_toc(abspath);
      if (toc) {
        bool any = false;
        for (auto const& mem : *toc) {
          if (!thumtoo::is_likely_image_member_path(mem.member_path)) {
            continue;
          }
          expanded.push_back(URL::from_string(
            "file://" + abspath.string() + "//archive:" + mem.member_path));
          any = true;
        }
        if (any) {
          continue;
        }
      }
    }
    expanded.push_back(u);
  }
  urls = std::move(expanded);
#endif

  int added = 0;
  for (URL const& u : urls) {
    TileProviderPtr provider;
#ifdef HAVE_THUMTOO
    if (m_thumtoo) {
      std::string const uri = thumtoo_uri_from_url(u);
      if (!uri.empty()) {
        if (auto sz = m_thumtoo->get_size(uri)) {
          provider = ThumtooTileProvider::create_from_size(
            m_thumtoo, uri, sz->width, sz->height);
        } else {
          provider = ThumtooTileProvider::create(m_thumtoo, uri);
        }
      }
    }
#endif
    m_workspace->add_image(
      std::make_shared<Image>(u, provider, &m_job_manager));
    ++added;
  }

  if (added > 0) {
    log_info("open_paths: added {} item(s)", added);
    layout_tight();
    zoom_to_selection();
    redraw();
  }
}

} // namespace galapix

/* EOF */
