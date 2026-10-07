// Galapix - an image viewer for large image collections
// Copyright (C) 2026 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include "app/key_bindings.hpp"

namespace galapix {

KeyBinding const*
find_key_binding(KeyBindings const& bindings, SDL_Keycode key, bool shift)
{
  KeyBinding const* fallback = nullptr;
  for (KeyBinding const& binding : bindings) {
    if (binding.key != key) {
      continue;
    }
    if (binding.shift == shift) {
      return &binding;
    }
    if (!binding.shift) {
      fallback = &binding;
    }
  }
  return fallback;
}

} // namespace galapix

/* EOF */
