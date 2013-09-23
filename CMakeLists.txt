project(thumbnailer CXX)
cmake_minimum_required(VERSION 2.8.9)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -Wextra")

set(SO_VERSION_MAJOR "1")
set(SO_VERSION_MINOR "0")
set(SO_VERSION_PATCH "0")

set(SO_VERSION "${SO_VERSION_MAJOR}.${SO_VERSION_MINOR}.${SO_VERSION_PATCH}")

# Increment this manually whenever breaking ABI.
# http://tldp.org/HOWTO/Program-Library-HOWTO/shared-libraries.html#AEN135
set(ABI_VERSION 0)

include(GNUInstallDirs)
set(LIBDIR ${CMAKE_INSTALL_LIBDIR})

include(FindPkgConfig)
pkg_check_modules(DEPS gstreamer-1.0 gstreamer-plugins-base-1.0 gstreamer-pbutils-1.0 gstreamer-app-1.0)
pkg_check_modules(IMG_DEPS gdk-pixbuf-2.0)

include_directories(${DEPS_INCLUDE_DIRS})
include_directories(${IMG_DEPS_INCLUDE_DIRS})
include_directories(include)

enable_testing()
add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(tools)
add_subdirectory(include)
