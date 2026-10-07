// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

// wstgui on top of the new Canvas: a GUIManager with widgets and a
// keyboard controlled menu, input comes from wstinput.
//
//   arrow keys, enter   navigate the menu
//   mouse               hover the widgets

#include <format>

#include <surf/palette.hpp>
#include <wstgui/button.hpp>
#include <wstgui/controller_def.hpp>
#include <wstgui/group_component.hpp>
#include <wstgui/gui_manager.hpp>
#include <wstgui/label.hpp>
#include <wstgui/menu.hpp>
#include <wstgui/menu_item.hpp>
#include <wstgui/root_component.hpp>
#include <wstgui/style.hpp>
#include <wstgui/text_view.hpp>
#include <wstinput/controller.hpp>
#include <wstinput/controller_description.hpp>
#include <wstinput/input_bindings.hpp>
#include <wstinput/input_manager.hpp>

#include "demo_app.hpp"

using namespace wstdisplay;

class GuiDemo : public wstdemo::DemoApp
{
public:
  GuiDemo(int argc, char** argv) :
    DemoApp("GUI", argc, argv),
    m_canvas(),
    m_input(),
    m_font(),
    m_small_font(),
    m_style(),
    m_gui(),
    m_menu(),
    m_status("use the arrow keys and enter in the menu")
  {}

  void init() override
  {
    wstinput::ControllerDescription desc;
    desc.add_pointer("pointer-x", MOUSE_MOTION_X);
    desc.add_pointer("pointer-y", MOUSE_MOTION_Y);
    desc.add_button("primary", PRIMARY_BUTTON);
    desc.add_button("secondary", SECONDARY_BUTTON);
    desc.add_button("escape", ESCAPE_BUTTON);
    desc.add_button("enter", ENTER_BUTTON);
    desc.add_button("menu-up", MENU_UP_BUTTON);
    desc.add_button("menu-down", MENU_DOWN_BUTTON);
    desc.add_button("menu-left", MENU_LEFT_BUTTON);
    desc.add_button("menu-right", MENU_RIGHT_BUTTON);
    desc.add_axis("y2-axis", Y2_AXIS);

    m_input = std::make_unique<wstinput::InputManagerSDL>(desc);
    auto& bindings = m_input->bindings();
    bindings.bind_mouse_motion(MOUSE_MOTION_X, 0, 0);
    bindings.bind_mouse_motion(MOUSE_MOTION_Y, 0, 1);
    bindings.bind_mouse_button(PRIMARY_BUTTON, 0, SDL_BUTTON_LEFT);
    bindings.bind_keyboard_button(ENTER_BUTTON, SDL_SCANCODE_RETURN);
    bindings.bind_keyboard_button(ESCAPE_BUTTON, SDL_SCANCODE_BACKSPACE);
    bindings.bind_keyboard_button(MENU_UP_BUTTON, SDL_SCANCODE_UP);
    bindings.bind_keyboard_button(MENU_DOWN_BUTTON, SDL_SCANCODE_DOWN);
    bindings.bind_keyboard_button(MENU_LEFT_BUTTON, SDL_SCANCODE_LEFT);
    bindings.bind_keyboard_button(MENU_RIGHT_BUTTON, SDL_SCANCODE_RIGHT);
    bindings.bind_keyboard_axis(Y2_AXIS, SDL_SCANCODE_PAGEUP, SDL_SCANCODE_PAGEDOWN);

    m_font = get_font_manager().load(get_data("Vera.ttf"), 20);
    m_small_font = get_font_manager().load(get_data("Vera.ttf"), 14);
    m_style = std::make_unique<wstgui::Style>(&get_font_manager().get(m_font));

    m_gui = std::make_unique<wstgui::GUIManager>(*m_style);
    auto* root = m_gui->get_root();

    auto label = std::make_unique<wstgui::Label>("wstgui widgets", root);
    label->set_geometry(geom::frect(40, 30, 400, 60));
    root->add_child(std::move(label));

    auto text = std::make_unique<wstgui::TextView>(root);
    text->set_geometry(geom::frect(40, 70, 600, 330));
    text->set_font(&get_font_manager().get(m_small_font));
    text->set_text("The GUI components draw into a Canvas now and take their input from a "
                   "wstinput Controller. Text is laid out by a <b>TextArea</b>, so markup works "
                   "here as well.\n\nPage up and page down scroll this text view. "
                   "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Morbi in consectetur "
                   "sapien. Pellentesque a cursus est. Vestibulum nec ultricies sapien, et tristique "
                   "dui. Donec pulvinar turpis sed risus eleifend, eget malesuada tortor lacinia. "
                   "Aliquam ut dolor purus. Mauris porta diam a iaculis aliquet.");
    root->add_child(std::move(text));

    auto ok = std::make_unique<wstgui::Button>("Ok", root);
    ok->set_geometry(geom::frect(480, 350, 600, 390));
    root->add_child(std::move(ok));
    auto cancel = std::make_unique<wstgui::Button>("Cancel", root);
    cancel->set_geometry(geom::frect(340, 350, 460, 390));
    root->add_child(std::move(cancel));

    m_menu = std::make_unique<wstgui::Menu>("Options", geom::frect(geom::fpoint(680, 70), geom::fsize(520, 320)),
                                            *m_style, false, root);
    m_menu->add_slider("Music Volume", 75, 0, 100, 10, [this](int v) { m_status = std::format("music volume: {}", v); });
    m_menu->add_slider("Sound Volume", 100, 0, 100, 10, [this](int v) { m_status = std::format("sound volume: {}", v); });
    auto& mode = m_menu->add_enum("Renderer", 0, [this](int v) { m_status = std::format("renderer: {}", v); });
    mode.add_pair(0, "GL 3.3");
    mode.add_pair(1, "GLES 2.0");
    m_menu->add_button("Apply", [this] { m_status = "apply pressed"; });

    // the last added child gets the keyboard focus
    root->add_child(m_menu->create_group());
  }

  void on_event(SDL_Event const& event) override
  {
    m_input->on_event(event);
  }

  void update(float delta) override
  {
    m_input->update(delta);
    m_gui->update(delta, m_input->get_controller());
    m_input->clear();
  }

  void draw(Renderer& renderer) override
  {
    m_canvas.clear();
    m_gui->draw(m_canvas);
    m_canvas.draw_text(get_font_manager().get(m_small_font), geom::fpoint(40, 440), m_status, surf::palette::lightgray);
    renderer.render(m_canvas, RenderPass{.clear = surf::Color(0.15f, 0.2f, 0.25f)});
  }

private:
  Canvas m_canvas;
  std::unique_ptr<wstinput::InputManagerSDL> m_input;
  FontId m_font;
  FontId m_small_font;
  std::unique_ptr<wstgui::Style> m_style;
  std::unique_ptr<wstgui::GUIManager> m_gui;
  std::unique_ptr<wstgui::Menu> m_menu;
  std::string m_status;
};

WST_DEMO_MAIN(GuiDemo)

/* EOF */
