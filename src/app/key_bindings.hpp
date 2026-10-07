// Galapix - an image viewer for large image collections
// Copyright (C) 2026 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#ifndef HEADER_GALAPIX_APP_KEY_BINDINGS_HPP
#define HEADER_GALAPIX_APP_KEY_BINDINGS_HPP

#include <functional>
#include <vector>

#include <SDL_keycode.h>

namespace galapix {

/** One keyboard shortcut. The same table drives the keyboard handling
    (AppViewer) and the Help panel (ImguiOverlay), so they can't drift. */
struct KeyBinding
{
  SDL_Keycode key;
  /** Only with Shift held; a binding without it also matches with Shift */
  bool shift;
  /** Key as shown in the Help panel, e.g. "Shift+s" */
  char const* label;
  /** Help panel section */
  char const* group;
  char const* description;
  std::function<void ()> action;
};

using KeyBindings = std::vector<KeyBinding>;

/** The binding for \a key: one requiring Shift when \a shift is held,
    else one without. nullptr when the key is unbound. */
KeyBinding const* find_key_binding(KeyBindings const& bindings, SDL_Keycode key, bool shift);

} // namespace galapix

#endif

/* EOF */
