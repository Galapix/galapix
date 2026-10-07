cmake_minimum_required(VERSION 3.14)

project(xdgcpp
  VERSION 0.1.0
  DESCRIPTION "XDG Base Directory Specification implementation in C++"
  LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

option(XDG_BUILD_TESTS "Build and register the xdgcpp unit-test suite" OFF)
option(XDG_BUILD_INFO  "Build the xdgcpp-info diagnostic tool" ON)

# Detect whether we are the top-level project or embedded via
# add_subdirectory().  Install rules and the info tool default only
# apply when we are top-level so that parent projects are not surprised.
set(XDGCPP_IS_TOP_LEVEL OFF)
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  set(XDGCPP_IS_TOP_LEVEL ON)
endif()

# ---------------------------------------------------------------------------
# Library
# ---------------------------------------------------------------------------

add_library(xdgcpp STATIC src/xdg.cpp)
add_library(xdgcpp::xdgcpp ALIAS xdgcpp)

set_target_properties(xdgcpp PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
  # Keep the output name stable for pkg-config / -lxdgcpp.
  OUTPUT_NAME xdgcpp
  EXPORT_NAME xdgcpp
)

target_include_directories(xdgcpp
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    # After install, headers live in include/xdgcpp/ so that
    #   #include <xdg.h>
    # works when the consumer adds that directory (via the exported
    # target or pkg-config).
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/xdgcpp>
)

# ---------------------------------------------------------------------------
# Optional diagnostic tool (top-level builds only by default)
# ---------------------------------------------------------------------------

if(XDG_BUILD_INFO AND XDGCPP_IS_TOP_LEVEL)
  add_executable(xdgcpp-info src/xdg_info.cpp)
  set_target_properties(xdgcpp-info PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
  )
  target_link_libraries(xdgcpp-info PRIVATE xdgcpp::xdgcpp)
endif()

# ---------------------------------------------------------------------------
# Optional unit tests
# ---------------------------------------------------------------------------

if(XDG_BUILD_TESTS)
  enable_testing()
  add_executable(xdgcpp_test src/xdg_test.cpp)
  set_target_properties(xdgcpp_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
  )
  target_link_libraries(xdgcpp_test PRIVATE xdgcpp::xdgcpp)

  add_test(NAME xdgcpp_test COMMAND xdgcpp_test)
  set_tests_properties(xdgcpp_test PROPERTIES TIMEOUT 30)
endif()

# ---------------------------------------------------------------------------
# Install / export (top-level only)
#
# When this project is consumed via add_subdirectory(), the parent already
# sees the xdgcpp / xdgcpp::xdgcpp targets and their BUILD_INTERFACE
# include directories.  Running install rules from an embedded copy is
# almost never desired, so they are gated on XDGCPP_IS_TOP_LEVEL.
# ---------------------------------------------------------------------------

if(XDGCPP_IS_TOP_LEVEL)
  install(TARGETS xdgcpp
    EXPORT xdgcpp-targets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  )

  install(FILES include/xdg.h
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/xdgcpp"
  )

  if(TARGET xdgcpp-info)
    install(TARGETS xdgcpp-info
      RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    )
  endif()

  # Generated CMake package config so find_package(xdgcpp) works.
  set(xdgcpp_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/xdgcpp")

  configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/xdgcpp-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/xdgcpp-config.cmake"
    INSTALL_DESTINATION "${xdgcpp_INSTALL_CMAKEDIR}"
  )

  write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/xdgcpp-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
  )

  install(EXPORT xdgcpp-targets
    FILE xdgcpp-targets.cmake
    NAMESPACE xdgcpp::
    DESTINATION "${xdgcpp_INSTALL_CMAKEDIR}"
  )

  install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/xdgcpp-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/xdgcpp-config-version.cmake"
    DESTINATION "${xdgcpp_INSTALL_CMAKEDIR}"
  )

  # pkg-config
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/pkgconfig/xdgcpp.pc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/xdgcpp.pc"
    @ONLY
  )
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/xdgcpp.pc"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig"
  )
endif()

# EOF #
