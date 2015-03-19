cmake_minimum_required(VERSION 3.0)
project(uitest)

find_package(Threads)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall -Wextra -Wshadow -Wno-unused-parameter")

# build gtest
add_library(gtest STATIC external/gtest-1.7.0/src/gtest-all.cc)
target_include_directories(gtest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/gtest-1.7.0/)

add_library(gtest_main STATIC external/gtest-1.7.0/src/gtest_main.cc)
target_include_directories(gtest_main PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/gtest-1.7.0/)

# build uitest
add_library(uitest STATIC src/arg_info.cpp src/uitest.cpp)
target_include_directories(uitest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(uitest_main STATIC src/uitest_main.cpp)
target_include_directories(uitest_main PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# build examples
add_executable(example_test examples/examples.cpp)
target_link_libraries(example_test LINK_PUBLIC uitest uitest_main)
target_include_directories(example_test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# build tests
add_executable(uitest_test test/arg_info_test.cpp)
target_include_directories(uitest_test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(uitest_test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/gtest-1.7.0/)
target_link_libraries(uitest_test LINK_PUBLIC uitest gtest gtest_main ${CMAKE_THREAD_LIBS_INIT})

# EOF #
