# Copyright (C) 2020 Ingo Ruhnke <grumbel@gmail.com>
#
# This software is provided 'as-is', without any express or implied
# warranty.  In no event will the authors be held liable for any damages
# arising from the use of this software.
#
# Permission is granted to anyone to use this software for any purpose,
# including commercial applications, and to alter it and redistribute it
# freely, subject to the following restrictions:
#
# 1. The origin of this software must not be misrepresented; you must not
#    claim that you wrote the original software. If you use this software
#    in a product, an acknowledgment in the product documentation would be
#    appreciated but is not required.
# 2. Altered source versions must be plainly marked as such, and must not be
#    misrepresented as being the original software.
# 3. This notice may not be removed or altered from any source distribution.

# Finds squirrel and sqstdlib and provides Squirrel::Squirrel.
#
# The search can be skipped by setting SQUIRREL_LIBRARY, SQSTDLIB_LIBRARY
# and SQUIRREL_INCLUDE_DIR, e.g. for cross builds.

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_search_module(PC_SQUIRREL QUIET squirrel3)
endif()

find_library(SQUIRREL_LIBRARY
  NAMES squirrel squirrel3 squirrel_static
  HINTS ${PC_SQUIRREL_LIBRARY_DIRS})
find_library(SQSTDLIB_LIBRARY
  NAMES sqstdlib sqstdlib3 sqstdlib_static
  HINTS ${PC_SQUIRREL_LIBRARY_DIRS})
find_path(SQUIRREL_INCLUDE_DIR
  NAMES squirrel.h
  PATH_SUFFIXES squirrel squirrel3
  HINTS ${PC_SQUIRREL_INCLUDE_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Squirrel
  REQUIRED_VARS SQUIRREL_LIBRARY SQSTDLIB_LIBRARY SQUIRREL_INCLUDE_DIR
  VERSION_VAR PC_SQUIRREL_VERSION)

if(Squirrel_FOUND)
  set(SQUIRREL_LIBRARIES ${SQUIRREL_LIBRARY} ${SQSTDLIB_LIBRARY})
  set(SQUIRREL_INCLUDE_DIRS ${SQUIRREL_INCLUDE_DIR})

  if(NOT TARGET Squirrel::Squirrel)
    add_library(Squirrel::Squirrel INTERFACE IMPORTED)
    target_link_libraries(Squirrel::Squirrel INTERFACE ${SQUIRREL_LIBRARIES})
    target_include_directories(Squirrel::Squirrel INTERFACE ${SQUIRREL_INCLUDE_DIRS})
  endif()
endif()

mark_as_advanced(SQUIRREL_LIBRARY SQSTDLIB_LIBRARY SQUIRREL_INCLUDE_DIR)

# EOF #
