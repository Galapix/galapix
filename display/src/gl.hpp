// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef HEADER_WSTDISPLAY_GL_HPP
#define HEADER_WSTDISPLAY_GL_HPP

// Internal header, the public headers never include GL headers.

#include <glad/gl.h>

#include <wstdisplay/caps.hpp>

namespace wstdisplay::detail {

/** Throws std::runtime_error when glGetError() reports an error */
void check_gl(char const* file, int line, char const* what);

/** Load the GL function pointers on the current context, detect the
    API and fill in the capabilities. Called by Device. */
Caps load_gl(void* (*loader)(char const* name));

char const* gl_error_string(GLenum error);

} // namespace wstdisplay::detail

#ifdef NDEBUG
#  define WST_CHECK_GL(what) do {} while(false)
#else
#  define WST_CHECK_GL(what) ::wstdisplay::detail::check_gl(__FILE__, __LINE__, (what))
#endif

#endif

/* EOF */
