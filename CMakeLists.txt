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

include(cmake/coverage.cmake)

include(GNUInstallDirs)
set(LIBDIR ${CMAKE_INSTALL_LIBDIR})
set(SHARE_PRIV_DIR ${CMAKE_INSTALL_LIBDIR}/thumbnailer)
set(SHARE_PRIV_ABS ${CMAKE_INSTALL_PREFIX}/${SHARE_PRIV_DIR})

include(FindPkgConfig)
pkg_check_modules(GST_DEPS REQUIRED gstreamer-1.0 gstreamer-plugins-base-1.0 gstreamer-pbutils-1.0 gstreamer-app-1.0)
pkg_check_modules(IMG_DEPS REQUIRED gdk-pixbuf-2.0)
pkg_check_modules(SOUP_DEPS REQUIRED libsoup-2.4)
pkg_check_modules(XML_DEPS REQUIRED libxml-2.0)

include_directories(${GST_DEPS_INCLUDE_DIRS})
include_directories(${IMG_DEPS_INCLUDE_DIRS})
include_directories(include)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)

enable_testing()
add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(tools)
add_subdirectory(include)
