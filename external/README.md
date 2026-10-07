<!--
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
SPDX-License-Identifier: GPL-3.0-or-later
-->

# external/

Place for **vendored** third-party sources when a dependency is not taken from
Nix/system packages.

System libraries (SQLite, libvips, libjxl, libarchive, …) are **not** vendored
here — they come from `flake.nix` / pkg-config.
