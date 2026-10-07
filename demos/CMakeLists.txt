# SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
# SPDX-License-Identifier: GPL-3.0-or-later

include(GNUInstallDirs)

add_library(wstdemo STATIC common/demo_app.cpp)
target_include_directories(wstdemo PUBLIC common)
target_link_libraries(wstdemo PUBLIC wstdisplay::wstdisplay)
target_compile_definitions(wstdemo PRIVATE
  WST_DEMO_SOURCE_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
  WST_DEMO_INSTALL_DATADIR="${CMAKE_INSTALL_FULL_DATADIR}/wst/demos")
target_compile_options(wstdemo PRIVATE ${WST_WARNINGS_CXX_FLAGS})

file(GLOB DEMO_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} demo_*.cpp)

# demos using more than wstdisplay
if(TARGET wstgui::wstgui)
  set(DEMO_EXTRA_LIBS_demo_gui wstgui::wstgui)
else()
  list(REMOVE_ITEM DEMO_SOURCES demo_gui.cpp)
endif()

if(TARGET wstsprite::wstsprite)
  set(DEMO_EXTRA_LIBS_demo_sprite_viewer wstsprite::wstsprite)
else()
  list(REMOVE_ITEM DEMO_SOURCES demo_sprite_viewer.cpp)
endif()
foreach(SRC ${DEMO_SOURCES})
  get_filename_component(NAME ${SRC} NAME_WE)
  string(REPLACE "_" "-" EXE_NAME "wst-${NAME}")
  add_executable(${NAME} ${SRC})
  set_target_properties(${NAME} PROPERTIES OUTPUT_NAME ${EXE_NAME})
  target_link_libraries(${NAME} PRIVATE wstdemo ${DEMO_EXTRA_LIBS_${NAME}})
  target_compile_options(${NAME} PRIVATE ${WST_WARNINGS_CXX_FLAGS})
  install(TARGETS ${NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
  list(APPEND WST_DEMO_TARGETS ${NAME})
endforeach()

install(DIRECTORY data/ DESTINATION ${CMAKE_INSTALL_DATADIR}/wst/demos)

# surf comes through wstdisplay, surf-config doesn't look up geom itself
add_executable(image_compare tools/image_compare.cpp)
set_target_properties(image_compare PROPERTIES OUTPUT_NAME wst-image-compare)
target_link_libraries(image_compare PRIVATE wstdisplay::wstdisplay)

# Render a few frames of every demo with each API, offscreen through
# EGL, so no display is needed (llvmpipe works as well)
if(BUILD_TESTS)
  foreach(NAME ${WST_DEMO_TARGETS})
    foreach(API gl gles)
      add_test(NAME ${NAME}-${API}
        COMMAND ${NAME} --${API} --hidden --size 640x360 --frames 30
                --screenshot ${CMAKE_CURRENT_BINARY_DIR}/${NAME}-${API}.png)
      set_tests_properties(${NAME}-${API} PROPERTIES
        LABELS "gl"
        TIMEOUT 60
        ENVIRONMENT "SDL_VIDEODRIVER=offscreen"
        FIXTURES_SETUP ${NAME}-screenshots)
    endforeach()

    # GL 3.3 and GLES 2.0 must render the same image
    add_test(NAME ${NAME}-gl-vs-gles
      COMMAND image_compare ${CMAKE_CURRENT_BINARY_DIR}/${NAME}-gl.png
                            ${CMAKE_CURRENT_BINARY_DIR}/${NAME}-gles.png 0.5)
    set_tests_properties(${NAME}-gl-vs-gles PROPERTIES
      LABELS "gl"
      FIXTURES_REQUIRED ${NAME}-screenshots)
  endforeach()

  if(TARGET demo_sprite_viewer)
    add_test(NAME demo_sprite_viewer-check
      COMMAND demo_sprite_viewer --check --hidden --frames 1)
    set_tests_properties(demo_sprite_viewer-check PROPERTIES
      LABELS "gl" TIMEOUT 60 ENVIRONMENT "SDL_VIDEODRIVER=offscreen"
      FAIL_REGULAR_EXPRESSION " [1-9][0-9]* failed|error:")
  endif()
endif()

# EOF #
