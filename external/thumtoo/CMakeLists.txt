# SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.16)

# Version (single source of truth: this tree's VERSION file).
#   Packaging (nix build top-level): -DPROJECT_VERSION_FULL=... or -DTHUMTOO_VERSION_FULL=...
#   Dev: VERSION + git rev-list count + short hash (same scheme as biltoo/dirtoo)
#
# When biltoo add_subdirectory()'s this tree, the parent already defines
# PROJECT_VERSION_FULL (biltoo's string). Must NOT inherit it — otherwise
# thumtoo::version_string() reports biltoo's rev/hash (e.g. identical
# "0.2.0-dev.1783+ga417a19" for both). Prefer THUMTOO_VERSION_FULL; only
# honor PROJECT_VERSION_FULL when thumtoo is the top-level project().
#
# Runtime API: generated thumtoo/version.hpp (not -D compile flags), so a VERSION
# bump only recompiles TUs that include that header — not the whole static lib.
set(_thumtoo_is_top_level FALSE)
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(_thumtoo_is_top_level TRUE)
endif()
if(DEFINED THUMTOO_VERSION_FULL AND NOT THUMTOO_VERSION_FULL STREQUAL "")
  string(STRIP "${THUMTOO_VERSION_FULL}" PROJECT_VERSION_FULL)
elseif(_thumtoo_is_top_level
       AND DEFINED PROJECT_VERSION_FULL
       AND NOT PROJECT_VERSION_FULL STREQUAL "")
  string(STRIP "${PROJECT_VERSION_FULL}" PROJECT_VERSION_FULL)
else()
  file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" PROJECT_VERSION_FULL LIMIT_COUNT 1)
  string(STRIP "${PROJECT_VERSION_FULL}" PROJECT_VERSION_FULL)

  if(PROJECT_VERSION_FULL MATCHES "-dev")
    find_package(Git QUIET)
    set(_thumtoo_git_dir "${CMAKE_CURRENT_SOURCE_DIR}/.git")
    if(Git_FOUND AND EXISTS "${_thumtoo_git_dir}")
      if(NOT IS_DIRECTORY "${_thumtoo_git_dir}")
        file(READ "${_thumtoo_git_dir}" _thumtoo_gitfile)
        string(REGEX REPLACE "^gitdir:[ \t]*([^\n]+).*" "\\1" _thumtoo_git_dir
               "${_thumtoo_gitfile}")
        if(NOT IS_ABSOLUTE "${_thumtoo_git_dir}")
          set(_thumtoo_git_dir "${CMAKE_CURRENT_SOURCE_DIR}/${_thumtoo_git_dir}")
        endif()
      endif()

      execute_process(
        COMMAND "${GIT_EXECUTABLE}" -C "${CMAKE_CURRENT_SOURCE_DIR}" rev-list --count HEAD
        OUTPUT_VARIABLE _thumtoo_git_count
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        RESULT_VARIABLE _thumtoo_git_count_rc
      )
      execute_process(
        COMMAND "${GIT_EXECUTABLE}" -C "${CMAKE_CURRENT_SOURCE_DIR}" rev-parse --short=7 HEAD
        OUTPUT_VARIABLE _thumtoo_git_rev
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        RESULT_VARIABLE _thumtoo_git_rev_rc
      )
      if(_thumtoo_git_count_rc EQUAL 0 AND _thumtoo_git_rev_rc EQUAL 0
         AND NOT _thumtoo_git_count STREQUAL "" AND NOT _thumtoo_git_rev STREQUAL "")
        set(PROJECT_VERSION_FULL
            "${PROJECT_VERSION_FULL}.${_thumtoo_git_count}+g${_thumtoo_git_rev}")
        message(STATUS "Dev version from git: ${PROJECT_VERSION_FULL}")
      endif()
      set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/VERSION"
        "${_thumtoo_git_dir}/HEAD"
        "${_thumtoo_git_dir}/index")
    else()
      set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/VERSION")
    endif()
  else()
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
      "${CMAKE_CURRENT_SOURCE_DIR}/VERSION")
  endif()
endif()

string(REGEX MATCH "^[0-9]+\\.[0-9]+\\.[0-9]+" PROJECT_VERSION_CMAKE "${PROJECT_VERSION_FULL}")
if(NOT PROJECT_VERSION_CMAKE)
  set(PROJECT_VERSION_CMAKE "0.0.0")
endif()

string(REGEX MATCH "^([0-9]+)\\.([0-9]+)\\.([0-9]+)" _thumtoo_ver_match "${PROJECT_VERSION_CMAKE}")
set(THUMTOO_VERSION_MAJOR "${CMAKE_MATCH_1}")
set(THUMTOO_VERSION_MINOR "${CMAKE_MATCH_2}")
set(THUMTOO_VERSION_PATCH "${CMAKE_MATCH_3}")
if(THUMTOO_VERSION_MAJOR STREQUAL "")
  set(THUMTOO_VERSION_MAJOR 0)
  set(THUMTOO_VERSION_MINOR 0)
  set(THUMTOO_VERSION_PATCH 0)
endif()
set(THUMTOO_VERSION_STRING "${PROJECT_VERSION_FULL}")

project(thumtoo VERSION ${PROJECT_VERSION_CMAKE} LANGUAGES C CXX)
message(STATUS "thumtoo version: ${PROJECT_VERSION_FULL}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(GNUInstallDirs)

option(THUMTOO_BUILD_TESTS "Build unit tests" ON)
option(THUMTOO_BUILD_TOOLS "Build CLI tools" ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SQLITE3 REQUIRED IMPORTED_TARGET sqlite3)
pkg_check_modules(VIPS REQUIRED IMPORTED_TARGET vips)
pkg_check_modules(LIBARCHIVE REQUIRED IMPORTED_TARGET libarchive)
# Optional solid-RAR backend (libunarr / unarr). RAR5 still limited upstream.
option(THUMTOO_WITH_UNARR "Use libunarr for RAR/CBR (solid)" ON)
pkg_check_modules(UNARR IMPORTED_TARGET libunarr)
if(NOT UNARR_FOUND)
  pkg_check_modules(UNARR IMPORTED_TARGET unarr)
endif()
if(THUMTOO_WITH_UNARR AND UNARR_FOUND)
  set(THUMTOO_HAVE_UNARR 1)
  message(STATUS "thumtoo: libunarr enabled (solid RAR/CBR)")
else()
  set(THUMTOO_HAVE_UNARR 0)
  if(THUMTOO_WITH_UNARR)
    message(STATUS "thumtoo: libunarr not found — RAR via libarchive only")
  endif()
endif()
# Export feature flags to parent only when nested (biltoo add_subdirectory).
# Standalone `cmake -S .` has no parent — bare PARENT_SCOPE is an author warning.
macro(thumtoo_export_to_parent _var)
  get_directory_property(_thumtoo_parent PARENT_DIRECTORY)
  if(_thumtoo_parent)
    set(${_var} "${${_var}}" PARENT_SCOPE)
  endif()
endmacro()

thumtoo_export_to_parent(THUMTOO_HAVE_UNARR)
# DjVuLibre ships ddjvuapi.pc (headers + libdjvulibre). On Nix the .pc lives
# in the .dev output — ensure djvulibre is on PKG_CONFIG_PATH (buildInputs).
pkg_check_modules(DJVULIBRE IMPORTED_TARGET ddjvuapi)
if(NOT DJVULIBRE_FOUND)
  pkg_check_modules(DJVULIBRE IMPORTED_TARGET djvulibre)
endif()
pkg_check_modules(LIBCURL IMPORTED_TARGET libcurl)

add_library(thumtoo
  src/store.cpp
  src/uri.cpp
  src/client.cpp
  src/activity.cpp
  src/layout.cpp
  src/image.cpp
  src/debug_overlay.cpp
  src/archive.cpp
  src/pdf.cpp
  src/epub.cpp
  src/djvu.cpp
  src/expand.cpp
  src/format.cpp
  src/build_stats.cpp
  src/lqip.cpp
  src/handsum.cpp
  external/basic_handsum_decode_impl.c
  src/network.cpp
  src/text.cpp
  src/ocr.cpp
  src/appearance.cpp
  src/xdg_thumbnail.cpp
  # Host-side tile LOD core (planner, loader, scheduler, draw plan), shared
  # by biltoo and Galapix. See docs/TILE_LOD.md.
  src/lod/lod_planner.cpp
  src/lod/draw_plan.cpp
  src/lod/tile_loader.cpp
  src/lod/tile_scheduler.cpp
  src/lod/tile_session.cpp
  src/lod/source_records.cpp
  src/lod/source_status.cpp
  src/lod/client_tile_backend.cpp
)
add_library(thumtoo::thumtoo ALIAS thumtoo)

if(THUMTOO_HAVE_UNARR)
  target_sources(thumtoo PRIVATE src/archive_unarr.cpp)
  target_link_libraries(thumtoo PUBLIC PkgConfig::UNARR)
  target_compile_definitions(thumtoo PUBLIC THUMTOO_HAVE_UNARR=1)
else()
  target_sources(thumtoo PRIVATE src/archive_unarr_stub.cpp)
endif()

# Generated version.hpp lives in the build tree (configure_file below).
target_include_directories(thumtoo
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(thumtoo PUBLIC
  PkgConfig::SQLITE3
  PkgConfig::VIPS
  PkgConfig::LIBARCHIVE
)
# MuPDF (required for PDF pages; Poppler support removed).
pkg_check_modules(MUPDF IMPORTED_TARGET mupdf)
if(NOT MUPDF_FOUND)
  find_path(MUPDF_INCLUDE_DIR mupdf/fitz.h)
  find_library(MUPDF_LIBRARY NAMES mupdf)
  if(MUPDF_INCLUDE_DIR AND MUPDF_LIBRARY)
    set(MUPDF_FOUND TRUE)
  endif()
endif()
set(THUMTOO_HAVE_MUPDF 0)
if(MUPDF_FOUND)
  if(TARGET PkgConfig::MUPDF)
    target_link_libraries(thumtoo PRIVATE PkgConfig::MUPDF)
  else()
    target_include_directories(thumtoo PRIVATE ${MUPDF_INCLUDE_DIR})
    target_link_libraries(thumtoo PRIVATE ${MUPDF_LIBRARY})
  endif()
  target_compile_definitions(thumtoo PUBLIC THUMTOO_HAVE_MUPDF=1)
  set(THUMTOO_HAVE_MUPDF 1)
  message(STATUS "thumtoo: MuPDF enabled (PDF pages)")
  target_sources(thumtoo PRIVATE src/pdf_mupdf.cpp)
else()
  message(STATUS "thumtoo: MuPDF not found — PDF page support disabled")
  # Stubs for symbols referenced from pdf.cpp / expand.cpp when MuPDF is absent.
  target_sources(thumtoo PRIVATE src/pdf_mupdf_stub.cpp)
endif()
thumtoo_export_to_parent(THUMTOO_HAVE_MUPDF)
thumtoo_export_to_parent(THUMTOO_HAVE_MUPDF)

# Optional Tesseract OCR (page text layers for scans).
option(THUMTOO_WITH_TESSERACT "Build with Tesseract OCR" ON)
pkg_check_modules(TESSERACT IMPORTED_TARGET tesseract)
set(THUMTOO_HAVE_TESSERACT 0)
if(THUMTOO_WITH_TESSERACT AND TESSERACT_FOUND)
  target_link_libraries(thumtoo PRIVATE PkgConfig::TESSERACT)
  target_compile_definitions(thumtoo PUBLIC THUMTOO_HAVE_TESSERACT=1)
  set(THUMTOO_HAVE_TESSERACT 1)
  message(STATUS "thumtoo: Tesseract enabled (OCR text layers)")
else()
  message(WARNING
    "thumtoo: Tesseract (tesseract.pc) not found — OCR text layers DISABLED. "
    "Install tesseract and ensure PKG_CONFIG_PATH sees tesseract.pc "
    "(and lept.pc from leptonica — tesseract Requires: lept). "
    "Nix: pkgs.tesseract + pkgs.leptonica via thumtoo.lib.mkBuildInputs. "
    "THUMTOO_HAVE_TESSERACT=0")
endif()
thumtoo_export_to_parent(THUMTOO_HAVE_TESSERACT)
message(STATUS "thumtoo: THUMTOO_HAVE_TESSERACT=${THUMTOO_HAVE_TESSERACT}")


set(THUMTOO_HAVE_DJVU 0)
if(DJVULIBRE_FOUND)
  # PRIVATE: headers do not expose ddjvuapi types; consumers only need thumtoo.
  target_link_libraries(thumtoo PRIVATE PkgConfig::DJVULIBRE)
  target_compile_definitions(thumtoo PUBLIC THUMTOO_HAVE_DJVU=1)
  set(THUMTOO_HAVE_DJVU 1)
  message(STATUS "thumtoo: DjVuLibre enabled (DjVu pages) includes=${DJVULIBRE_INCLUDE_DIRS}")
else()
  message(WARNING
    "thumtoo: DjVuLibre (ddjvuapi.pc) not found — multipage DjVu will NOT expand. "
    "Install djvulibre/libdjvulibre-dev and ensure PKG_CONFIG_PATH sees ddjvuapi.pc. "
    "Nix: put pkgs.djvulibre in buildInputs (pulls .dev for pkg-config).")
endif()
thumtoo_export_to_parent(THUMTOO_HAVE_DJVU)

set(THUMTOO_HAVE_CURL 0)
if(LIBCURL_FOUND)
  target_link_libraries(thumtoo PRIVATE PkgConfig::LIBCURL)
  target_compile_definitions(thumtoo PUBLIC THUMTOO_HAVE_CURL=1)
  set(THUMTOO_HAVE_CURL 1)
  message(STATUS "thumtoo: libcurl enabled (HTTP/S source fetch)")
else()
  message(STATUS "thumtoo: libcurl not found — HTTP/S fetch disabled")
endif()
thumtoo_export_to_parent(THUMTOO_HAVE_CURL)

# Optional: session-bus Thumbnailer1 (XDG thumbnails). Cache paths work without it.
option(THUMTOO_WITH_DBUS "XDG Thumbnailer1 via libdbus-1" ON)
set(THUMTOO_HAVE_DBUS 0)
if(THUMTOO_WITH_DBUS)
  pkg_check_modules(DBUS IMPORTED_TARGET dbus-1)
  if(DBUS_FOUND)
    set(THUMTOO_HAVE_DBUS 1)
  endif()
endif()
thumtoo_export_to_parent(THUMTOO_HAVE_DBUS)


if(THUMTOO_HAVE_DBUS)
  target_link_libraries(thumtoo PUBLIC PkgConfig::DBUS)
  target_compile_definitions(thumtoo PUBLIC THUMTOO_HAVE_DBUS=1)
else()
  target_compile_definitions(thumtoo PUBLIC THUMTOO_HAVE_DBUS=0)
endif()

# version.hpp after all THUMTOO_HAVE_* are set (optional feature macros).
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/include/thumtoo/version.hpp.in"
  "${CMAKE_CURRENT_BINARY_DIR}/include/thumtoo/version.hpp"
  @ONLY)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/include/thumtoo/version.hpp"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/thumtoo")

target_compile_options(thumtoo PRIVATE -Wall -Wextra -Wpedantic)

if(THUMTOO_BUILD_TOOLS)
  add_executable(thumtoo-status tools/thumtoo_status.cpp)
  target_link_libraries(thumtoo-status PRIVATE thumtoo)
  add_executable(thumtoo-prepare tools/thumtoo_prepare.cpp)
  target_link_libraries(thumtoo-prepare PRIVATE thumtoo)
  add_executable(thumtoo-bench tools/thumtoo_bench.cpp)
  add_executable(thumtoo-tile tools/thumtoo_tile.cpp)
  add_executable(thumtoo-export tools/thumtoo_export.cpp)
  add_executable(thumtoo-gc tools/thumtoo_gc.cpp)
  target_link_libraries(thumtoo-gc PRIVATE thumtoo)

  add_executable(thumtoo-xdg-thumb tools/thumtoo_xdg_thumb.cpp)
  target_link_libraries(thumtoo-xdg-thumb PRIVATE thumtoo)
  add_executable(thumtoo-archive tools/thumtoo_archive.cpp)
  target_link_libraries(thumtoo-archive PRIVATE thumtoo)
  target_link_libraries(thumtoo-tile PRIVATE thumtoo)
  target_link_libraries(thumtoo-export PRIVATE thumtoo)
  target_link_libraries(thumtoo-bench PRIVATE thumtoo)
  add_executable(thumtoo-page-profile tools/thumtoo_page_profile.cpp)
  target_link_libraries(thumtoo-page-profile PRIVATE thumtoo)
  # The golden-path tools use only libvips / libarchive / libunarr, not the
  # thumtoo library; they take the generated version.hpp (header-only) for
  # --version and share the CLI/output layers in tools/golden/.
  add_executable(thumtoo-microbench-decode tools/microbench_decode.cpp)
  target_link_libraries(thumtoo-microbench-decode PRIVATE PkgConfig::VIPS)
  add_executable(thumtoo-gp-tile tools/golden/gp_tile.cpp)
  target_link_libraries(thumtoo-gp-tile PRIVATE PkgConfig::VIPS)
  add_executable(thumtoo-gp-archive tools/golden/gp_archive.cpp)
  target_link_libraries(thumtoo-gp-archive PRIVATE PkgConfig::LIBARCHIVE)
  foreach(gp_tool thumtoo-microbench-decode thumtoo-gp-tile thumtoo-gp-archive)
    target_include_directories(${gp_tool} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/include)
    target_compile_options(${gp_tool} PRIVATE -Wall -Wextra)
  endforeach()
  if(THUMTOO_HAVE_UNARR)
    target_link_libraries(thumtoo-gp-archive PRIVATE PkgConfig::UNARR)
    target_compile_definitions(thumtoo-gp-archive PRIVATE THUMTOO_HAVE_UNARR=1)
  endif()
endif()

if(THUMTOO_BUILD_TESTS)
  enable_testing()

  add_executable(thumtoo-test-store tests/test_store.cpp)
  target_link_libraries(thumtoo-test-store PRIVATE thumtoo)
  add_test(NAME store COMMAND thumtoo-test-store)

  add_executable(thumtoo-test-xdg-thumbnail tests/test_xdg_thumbnail.cpp)
  target_link_libraries(thumtoo-test-xdg-thumbnail PRIVATE thumtoo)
  add_test(NAME xdg_thumbnail COMMAND thumtoo-test-xdg-thumbnail)
  add_executable(thumtoo-test-store-root tests/test_store_root.cpp)
  target_link_libraries(thumtoo-test-store-root PRIVATE thumtoo)
  add_test(NAME store_root COMMAND thumtoo-test-store-root)
  add_executable(thumtoo-test-store-only tests/test_store_only.cpp)
  target_link_libraries(thumtoo-test-store-only PRIVATE thumtoo)
  add_test(NAME store_only COMMAND thumtoo-test-store-only)
  add_executable(thumtoo-test-client tests/test_client.cpp)
  target_link_libraries(thumtoo-test-client PRIVATE thumtoo)
  add_test(NAME client COMMAND thumtoo-test-client)
  add_executable(thumtoo-test-uri tests/test_uri.cpp)
  add_executable(thumtoo-test-lqip tests/test_lqip.cpp)
  target_link_libraries(thumtoo-test-lqip PRIVATE thumtoo)
  add_test(NAME lqip COMMAND thumtoo-test-lqip)
  target_link_libraries(thumtoo-test-uri PRIVATE thumtoo)
  add_test(NAME uri COMMAND thumtoo-test-uri)
  add_executable(thumtoo-test-lod tests/test_lod.cpp)
  target_link_libraries(thumtoo-test-lod PRIVATE thumtoo)
  add_test(NAME lod COMMAND thumtoo-test-lod)
  add_executable(thumtoo-test-lod-client-backend tests/test_lod_client_backend.cpp)
  target_link_libraries(thumtoo-test-lod-client-backend PRIVATE thumtoo)
  add_test(NAME lod_client_backend COMMAND thumtoo-test-lod-client-backend)
  add_executable(thumtoo-test-appearance tests/test_appearance.cpp)
  target_link_libraries(thumtoo-test-appearance PRIVATE thumtoo)
  add_test(NAME appearance COMMAND thumtoo-test-appearance)

  add_executable(thumtoo-test-pdf-scale tests/test_pdf_scale.cpp)
  target_link_libraries(thumtoo-test-pdf-scale PRIVATE thumtoo)
  add_test(NAME pdf_scale COMMAND thumtoo-test-pdf-scale)

  add_executable(thumtoo-test-pdf-tiles tests/test_pdf_tiles.cpp)
  target_link_libraries(thumtoo-test-pdf-tiles PRIVATE thumtoo)
  add_test(NAME pdf_tiles COMMAND thumtoo-test-pdf-tiles)
  add_executable(thumtoo-test-soft-ladder tests/test_soft_ladder.cpp)
  target_link_libraries(thumtoo-test-soft-ladder PRIVATE thumtoo)
  add_test(NAME soft_ladder COMMAND thumtoo-test-soft-ladder)
  add_executable(thumtoo-test-pixels-from-tiles tests/test_pixels_from_tiles.cpp)
  target_link_libraries(thumtoo-test-pixels-from-tiles PRIVATE thumtoo)
  add_test(NAME pixels_from_tiles COMMAND thumtoo-test-pixels-from-tiles)
  add_executable(thumtoo-test-archive-cursor tests/test_archive_cursor.cpp)
  target_link_libraries(thumtoo-test-archive-cursor PRIVATE thumtoo)
  add_test(NAME archive_cursor COMMAND thumtoo-test-archive-cursor)
  add_executable(thumtoo-test-interest-cancel tests/test_interest_cancel.cpp)
  target_link_libraries(thumtoo-test-interest-cancel PRIVATE thumtoo)
  add_test(NAME interest_cancel COMMAND thumtoo-test-interest-cancel)
  add_executable(thumtoo-test-set-interest tests/test_set_interest.cpp)
  target_link_libraries(thumtoo-test-set-interest PRIVATE thumtoo)
  add_test(NAME set_interest COMMAND thumtoo-test-set-interest)
  add_executable(thumtoo-test-queue-stats tests/test_queue_stats.cpp)
  target_link_libraries(thumtoo-test-queue-stats PRIVATE thumtoo)
  add_test(NAME queue_stats COMMAND thumtoo-test-queue-stats)
  add_executable(thumtoo-test-activity tests/test_activity.cpp)
  target_link_libraries(thumtoo-test-activity PRIVATE thumtoo)
  add_test(NAME activity COMMAND thumtoo-test-activity)
  add_executable(thumtoo-test-tile-supersede-activity tests/test_tile_supersede_activity.cpp)
  target_link_libraries(thumtoo-test-tile-supersede-activity PRIVATE thumtoo)
  add_test(NAME tile_supersede_activity COMMAND thumtoo-test-tile-supersede-activity)
  add_executable(thumtoo-test-tile-cells-contract tests/test_tile_cells_contract.cpp)
  target_link_libraries(thumtoo-test-tile-cells-contract PRIVATE thumtoo)
  add_test(NAME tile_cells_contract COMMAND thumtoo-test-tile-cells-contract)
  if(THUMTOO_HAVE_MUPDF)
    # Builds its fixture PDFs with MuPDF's writer (tests/pdf_fixtures.cpp).
    add_executable(thumtoo-test-pdf-profile tests/test_pdf_profile.cpp tests/pdf_fixtures.cpp)
    target_link_libraries(thumtoo-test-pdf-profile PRIVATE thumtoo)
    if(TARGET PkgConfig::MUPDF)
      target_link_libraries(thumtoo-test-pdf-profile PRIVATE PkgConfig::MUPDF)
    else()
      target_include_directories(thumtoo-test-pdf-profile PRIVATE ${MUPDF_INCLUDE_DIR})
      target_link_libraries(thumtoo-test-pdf-profile PRIVATE ${MUPDF_LIBRARY})
    endif()
    add_test(NAME pdf_profile COMMAND thumtoo-test-pdf-profile)
    add_executable(thumtoo-test-epub-tiles tests/test_epub_tiles.cpp)
    target_link_libraries(thumtoo-test-epub-tiles PRIVATE thumtoo)
    target_compile_definitions(thumtoo-test-epub-tiles PRIVATE
      THUMTOO_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures")
    add_test(NAME epub_tiles COMMAND thumtoo-test-epub-tiles)
  endif()
  if(THUMTOO_HAVE_DJVU)
    add_executable(thumtoo-test-djvu-tiles tests/test_djvu_tiles.cpp)
    target_link_libraries(thumtoo-test-djvu-tiles PRIVATE thumtoo)
    target_compile_definitions(thumtoo-test-djvu-tiles PRIVATE
      THUMTOO_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures")
    add_test(NAME djvu_tiles COMMAND thumtoo-test-djvu-tiles)
  endif()
  add_executable(thumtoo-test-pdf-text tests/test_pdf_text.cpp)
  target_link_libraries(thumtoo-test-pdf-text PRIVATE thumtoo)
  add_test(NAME pdf_text COMMAND thumtoo-test-pdf-text)
  add_executable(thumtoo-test-djvu-text tests/test_djvu_text.cpp)
  target_link_libraries(thumtoo-test-djvu-text PRIVATE thumtoo)
  add_test(NAME djvu_text COMMAND thumtoo-test-djvu-text)
  target_compile_definitions(thumtoo-test-djvu-text PRIVATE
    THUMTOO_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures")
  add_executable(thumtoo-test-epub-text tests/test_epub_text.cpp)
  target_link_libraries(thumtoo-test-epub-text PRIVATE thumtoo)
  add_test(NAME epub_text COMMAND thumtoo-test-epub-text)
  add_executable(thumtoo-test-text-layer-cache tests/test_text_layer_cache.cpp)
  target_link_libraries(thumtoo-test-text-layer-cache PRIVATE thumtoo)
  add_test(NAME text_layer_cache COMMAND thumtoo-test-text-layer-cache)
  # Golden-tool verdict logic (header-only; no thumtoo linkage).
  add_executable(thumtoo-test-gp-verdict tests/test_gp_verdict.cpp)
  target_include_directories(thumtoo-test-gp-verdict PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/tools/golden)
  add_test(NAME gp_verdict COMMAND thumtoo-test-gp-verdict)
  add_executable(thumtoo-test-gp-json tests/test_gp_json.cpp)
  target_include_directories(thumtoo-test-gp-json PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/tools/golden)
  add_test(NAME gp_json COMMAND thumtoo-test-gp-json)
  add_executable(thumtoo-test-gp-cli tests/test_gp_cli.cpp)
  target_include_directories(thumtoo-test-gp-cli PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/tools/golden)
  add_test(NAME gp_cli COMMAND thumtoo-test-gp-cli)
  add_executable(thumtoo-test-gp-output tests/test_gp_output.cpp)
  target_include_directories(thumtoo-test-gp-output PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/tools/golden)
  add_test(NAME gp_output COMMAND thumtoo-test-gp-output)

  # Command-line behavior of the benchmark tools, end to end (exit codes, CSV /
  # JSON shape, failure handling). Needs the tools and python3.
  if(THUMTOO_BUILD_TOOLS)
    find_package(Python3 COMPONENTS Interpreter)
    if(Python3_Interpreter_FOUND)
      add_test(NAME bench_tools_cli
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_bench_tools_cli.py
          --gp-archive $<TARGET_FILE:thumtoo-gp-archive>
          --gp-tile $<TARGET_FILE:thumtoo-gp-tile>
          --microbench-decode $<TARGET_FILE:thumtoo-microbench-decode>
          --thumtoo-bench $<TARGET_FILE:thumtoo-bench>)
    endif()
  endif()
endif()

install(TARGETS thumtoo
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY include/thumtoo DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
if(THUMTOO_BUILD_TOOLS)
  install(TARGETS thumtoo-tile thumtoo-export thumtoo-status thumtoo-prepare thumtoo-bench thumtoo-xdg-thumb
                  thumtoo-gc thumtoo-archive thumtoo-microbench-decode
                  thumtoo-gp-tile thumtoo-gp-archive
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
  install(FILES
    man/thumtoo-prepare.1
    man/thumtoo-status.1
    man/thumtoo-tile.1
    man/thumtoo-export.1
    man/thumtoo-gc.1
    man/thumtoo-archive.1
    man/thumtoo-bench.1
    DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
  install(FILES
    man/thumtoo.7
    DESTINATION ${CMAKE_INSTALL_MANDIR}/man7)
endif()

# pkg-config for consumers (biltoo, etc.). Static lib: Requires.private lists public deps.
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/thumtoo.pc.in"
  "${CMAKE_CURRENT_BINARY_DIR}/thumtoo.pc"
  @ONLY)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/thumtoo.pc"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")

# Packaging environments (Nix cmake setup, etc.) pass the full GNUInstallDirs
# set and CMAKE_EXPORT_NO_PACKAGE_REGISTRY. Reference them so configure does
# not warn about unused -D variables for dirs we do not install into yet.
set(_thumtoo_pkg_vars_used
  "${CMAKE_INSTALL_SBINDIR}"
  "${CMAKE_INSTALL_LIBEXECDIR}"
  "${CMAKE_INSTALL_LOCALEDIR}"
  "${CMAKE_INSTALL_INFODIR}"
  "${CMAKE_INSTALL_DOCDIR}"
  "${CMAKE_INSTALL_MANDIR}"
  "${CMAKE_EXPORT_NO_PACKAGE_REGISTRY}")
unset(_thumtoo_pkg_vars_used)

# ---------------------------------------------------------------------------
# Feature summary (configure-time)
# ---------------------------------------------------------------------------
message(STATUS "")
message(STATUS "=== thumtoo feature summary ===")
# Value column aligned under the longest label below.
if(MUPDF_FOUND)
  message(STATUS "  PDF/EPUB backend (MuPDF):    ON     (//page:N, //mupdf-page:N)")
else()
  message(STATUS "  PDF/EPUB backend (MuPDF):    OFF    (install mupdf)")
endif()
if(DJVULIBRE_FOUND)
  message(STATUS "  DjVu pages (DjVuLibre):      ON")
else()
  message(STATUS "  DjVu pages (DjVuLibre):      OFF    (install djvulibre / ddjvuapi.pc)")
endif()
if(THUMTOO_HAVE_TESSERACT)
  message(STATUS "  OCR text layers (Tesseract): ON")
else()
  message(STATUS "  OCR text layers (Tesseract): OFF    (tesseract.pc + lept.pc)")
endif()
if(LIBCURL_FOUND)
  message(STATUS "  HTTP(S) fetch (libcurl):     ON")
else()
  message(STATUS "  HTTP(S) fetch (libcurl):     OFF")
endif()
message(STATUS "  Archives (libarchive):       ON")
if(THUMTOO_HAVE_UNARR)
  message(STATUS "  Solid RAR (libunarr):        ON")
else()
  message(STATUS "  Solid RAR (libunarr):        OFF    (RAR via libarchive only)")
endif()
message(STATUS "  Ladder/tiles (libvips):      ON")
if(THUMTOO_HAVE_DBUS)
  message(STATUS "  XDG Thumbnailer1 (dbus-1):   ON")
else()
  message(STATUS "  XDG Thumbnailer1 (dbus-1):   OFF    (cache path only)")
endif()
message(STATUS "  SQLite cache:                ON")
message(STATUS "================================")
message(STATUS "")
