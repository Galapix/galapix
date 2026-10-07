// SPDX-FileCopyrightText: 2005 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <wstgui/screen_manager.hpp>

#ifdef __EMSCRIPTEN__
#  include <emscripten.h>
#endif

#include <logmich/log.hpp>

#include <surf/save.hpp>
#include <wstinput/input_manager.hpp>
#include <wstdisplay/canvas.hpp>
#include <wstdisplay/opengl_window.hpp>
#include <wstdisplay/renderer.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <wstgui/screen.hpp>

namespace wstgui {

ScreenManager::ScreenManager(wstsystem::System& system, wstdisplay::OpenGLWindow& window, wstinput::InputManagerSDL& input) :
  m_system(system),
  m_window(window),
  m_input(input),
  m_screens(),
  m_screen_action(ScreenAction::NONE),
  m_screen_screen(),
  m_overlay_screens(),
  m_overlay_screen_action(ScreenAction::NONE),
  m_overlay_screen_screen(),
  m_ticks(0),
  m_overlap_delta(0),
  m_do_quit(false),
  m_key_bindings(),
  m_huds(),
  m_sig_update(),
  m_sig_draw_begin(),
  m_sig_draw_end(),
  m_canvas(std::make_unique<wstdisplay::Canvas>()),
  m_clear_color(0.0f, 0.0f, 0.0f)
{
  m_system.sig_event().connect([this](SDL_Event const& ev){ handle_event(ev); });
}

ScreenManager::~ScreenManager()
{
  m_screens.clear();
  m_overlay_screens.clear();
}

void
ScreenManager::run()
{
  m_do_quit = false;

  m_ticks = m_system.get_ticks();

  apply_pending_actions();

#ifdef __EMSCRIPTEN__
  // The browser calls run_frame() on every animation frame. run() doesn't
  // return, as the screens and the game state on the caller's stack must
  // stay alive while the loop runs.
  emscripten_set_main_loop_arg([](void* arg) {
    ScreenManager& self = *static_cast<ScreenManager*>(arg);
    if (self.m_do_quit || self.m_screens.empty()) {
      emscripten_cancel_main_loop();
    } else {
      self.run_frame();
    }
  }, this, 0, true);
#else
  while (!m_do_quit && !m_screens.empty())
  {
    run_frame();
    m_system.delay(5);
  }
#endif
}

void
ScreenManager::run_frame()
{
  /// Amount of time the world moves forward each update(), this is
  /// independed of the number of frames and always constant
  static const float step = 0.001f;

  Uint32 const now = m_system.get_ticks();
  float delta = static_cast<float>(now - m_ticks) / 1000.0f + m_overlap_delta;
  m_ticks = now;

  while (delta > step)
  {
    m_input.update(delta);

    if (!m_overlay_screens.empty()) {
      m_overlay_screens.back()->update(step, m_input.get_controller());
    } else if (!m_screens.empty()) {
      m_screens.back()->update(step, m_input.get_controller());
    }

    for(Screen* hud : m_huds) {
      hud->update(step, m_input.get_controller());
    }
    m_input.clear();

    delta -= step;
  }

  m_overlap_delta = delta;

  m_sig_update(delta);
  m_system.update();

  draw();

  apply_pending_actions();
}

void
ScreenManager::draw()
{
  wstdisplay::Renderer& renderer = m_window.get_renderer();
  geom::isize const window_size = m_window.get_size();
  geom::isize const drawable_size = m_window.get_drawable_size();

  wstdisplay::Canvas& canvas = *m_canvas;
  canvas.clear();

  m_sig_draw_begin(canvas);

  if (!m_screens.empty()) {
    m_screens.back()->draw(canvas);
  }

  if (!m_overlay_screens.empty()) {
    m_overlay_screens.back()->draw(canvas);
  }

  for(Screen* hud : m_huds) {
    hud->draw(canvas);
  }

  m_sig_draw_end(canvas);

  // the GUI is laid out in window coordinates, scale to the drawable for high-DPI
  glm::mat4 const scale = glm::scale(glm::mat4(1.0f),
                                     glm::vec3(static_cast<float>(drawable_size.width()) / static_cast<float>(window_size.width()),
                                               static_cast<float>(drawable_size.height()) / static_cast<float>(window_size.height()),
                                               1.0f));

  renderer.begin_frame(drawable_size);
  renderer.clear(m_clear_color);

  if (!m_screens.empty()) {
    m_screens.back()->render(renderer);
  }

  if (!m_overlay_screens.empty()) {
    m_overlay_screens.back()->render(renderer);
  }

  for(Screen* hud : m_huds) {
    hud->render(renderer);
  }

  renderer.render(canvas, wstdisplay::RenderPass{
      .world_matrix = scale,
      .screen_matrix = scale
    });
  renderer.end_frame();

  m_window.swap_buffers();
}

void
ScreenManager::apply_pending_actions()
{
  // Commit any pending screen actions
  switch (m_overlay_screen_action)
  {
    case ScreenAction::PUSH_SCREEN:
      m_overlay_screens.push_back(m_overlay_screen_screen);
      m_overlay_screen_screen = std::shared_ptr<Screen>();
      break;

    case ScreenAction::POP_SCREEN:
      if (m_overlay_screens.empty())
      {
        log_error("ScreenManager: trying to pop_overlay with empty stack");
      }
      else
      {
        m_overlay_screens.pop_back();
      }
      break;

    case ScreenAction::CLEAR_SCREENS:
      m_overlay_screens.clear();
      break;

    case ScreenAction::NONE:
      // nothing
      break;
  }

  m_overlay_screen_action = ScreenAction::NONE;

  switch (m_screen_action)
  {
    case ScreenAction::PUSH_SCREEN:
      m_screens.push_back(m_screen_screen);
      m_screen_screen = std::shared_ptr<Screen>();
      m_screens.back()->on_startup();
      break;

    case ScreenAction::POP_SCREEN:
      m_screens.pop_back();
      if (!m_screens.empty())
        m_screens.back()->on_startup();
      break;

    case ScreenAction::CLEAR_SCREENS:
      m_screens.clear();
      break;

    case ScreenAction::NONE:
      // nothing
      break;
  }
  m_screen_action = ScreenAction::NONE;

  while (!m_overlay_screens.empty() &&
         m_overlay_screens.back()->is_finished()) {
    m_overlay_screens.pop_back();
  }

  while (!m_screens.empty() &&
         m_screens.back()->is_finished()) {
    m_screens.pop_back();
  }
}

void
ScreenManager::handle_event(const SDL_Event& event)
{
  // pointer and touch input reaches the HUDs too, e.g. an on-screen
  // gamepad, in addition to the screens below
  switch(event.type)
  {
    case SDL_FINGERDOWN:
    case SDL_FINGERMOTION:
    case SDL_FINGERUP:
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
    case SDL_MOUSEMOTION:
      for(Screen* hud : m_huds) {
        hud->handle_event(event);
      }
      break;

    default:
      break;
  }

  switch(event.type)
  {
    case SDL_QUIT:
      // FIXME: This should be a bit more gentle, but will do for now
      log_info("Ctrl-c or Window-close pressed, game is going to quit");
      quit();
      break;

    case SDL_KEYDOWN:
    case SDL_KEYUP:
      if (event.key.state) {
        if (auto binding = m_key_bindings.find(event.key.keysym.sym); binding != m_key_bindings.end()) {
          binding->second();
        } else {
          if (!m_overlay_screens.empty()) {
            m_overlay_screens.back()->handle_event(event);
          } else if (!m_screens.empty()) {
            m_screens.back()->handle_event(event);
          }
        }
      }
      break;

    case SDL_MOUSEBUTTONUP:
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEMOTION:
    case SDL_JOYAXISMOTION:
    case SDL_JOYBALLMOTION:
    case SDL_JOYHATMOTION:
    case SDL_JOYBUTTONUP:
    case SDL_JOYBUTTONDOWN:
    case SDL_TEXTINPUT:
    case SDL_TEXTEDITING:
      if (!m_overlay_screens.empty()) {
        m_overlay_screens.back()->handle_event(event);
      }
      break;

    default:
      if (!m_overlay_screens.empty()) {
        m_overlay_screens.back()->handle_event(event);
      } else if (!m_screens.empty()) {
        m_screens.back()->handle_event(event);
      }
      break;
  }

  m_input.on_event(event);
}

void
ScreenManager::push_screen(std::unique_ptr<Screen> s)
{
  assert(m_screen_screen == nullptr);

  m_screen_action = ScreenAction::PUSH_SCREEN;
  m_screen_screen = std::shared_ptr<Screen>(std::move(s));
}

void
ScreenManager::pop_screen()
{
  m_screen_action = ScreenAction::POP_SCREEN;
}

void
ScreenManager::push_overlay(std::unique_ptr<Screen> s)
{
  assert(!m_overlay_screen_screen.get());

  m_overlay_screen_action = ScreenAction::PUSH_SCREEN;
  m_overlay_screen_screen = std::shared_ptr<Screen>(std::move(s));
}

void
ScreenManager::pop_overlay()
{
  m_overlay_screen_action = ScreenAction::POP_SCREEN;
}

void
ScreenManager::clear_overlay()
{
  m_overlay_screen_action = ScreenAction::CLEAR_SCREENS;
}

void
ScreenManager::quit()
{
  m_do_quit = true;
}

void
ScreenManager::bind_key(SDL_Keycode code, std::function<void()> callback)
{
  m_key_bindings[code] = std::move(callback);
}

void
ScreenManager::add_hud(Screen* screen)
{
  m_huds.emplace_back(screen);
}

void
ScreenManager::remove_hud(Screen* screen)
{
  std::erase(m_huds, screen);
}

} // namespace wstgui

/* EOF */
