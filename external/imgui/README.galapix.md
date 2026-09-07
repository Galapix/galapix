<!--
SPDX-FileCopyrightText: 2008-2026 Ingo Ruhnke <grumbel@gmail.com>
SPDX-License-Identifier: GPL-3.0-or-later
-->

# Dear ImGui (vendored)

Upstream: https://github.com/ocornut/imgui (v1.91.6)

License: MIT — see `LICENSE.txt` (ocornut / Dear ImGui contributors).

Galapix uses the core + `imgui_impl_sdl2` + `imgui_impl_opengl3` backends as a
**placeholder chrome** overlay on the pure OpenGL SDL viewer. Spatial image
interaction stays in Galapix; ImGui only shows status / future tool panels.
