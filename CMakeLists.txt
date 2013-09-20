project(thumbnailer CXX)
cmake_minimum_required(VERSION 2.8.9)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -Wextra")

include(FindPkgConfig)
pkg_check_modules(DEPS gstreamer-1.0 gstreamer-plugins-base-1.0 gstreamer-pbutils-1.0 gstreamer-app-1.0)
pkg_check_modules(IMG_DEPS gdk-pixbuf-2.0)

include_directories(${DEPS_INCLUDE_DIRS})
include_directories(${IMG_DEPS_INCLUDE_DIRS})
include_directories(include)

enable_testing()
add_subdirectory(src)
add_subdirectory(tests)
