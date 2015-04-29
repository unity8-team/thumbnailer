project(thumbnailer C CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

cmake_minimum_required(VERSION 2.8.7)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -Wextra")

include(CheckCXXCompilerFlag)
check_cxx_compiler_flag(-std=c++11 HAS_C11)
# Precise's GCC is too old to have -std=c++11
if(HAS_C11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
else()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -Dfinal=")
endif()

include(cmake/coverage.cmake)
include(cmake/UseGSettings.cmake)

include(GNUInstallDirs)
set(LIBDIR ${CMAKE_INSTALL_LIBDIR})
set(SHARE_PRIV_DIR ${CMAKE_INSTALL_LIBDIR}/thumbnailer)
set(SHARE_PRIV_ABS ${CMAKE_INSTALL_PREFIX}/${SHARE_PRIV_DIR})

find_package(Boost COMPONENTS filesystem system REQUIRED)
find_package(Threads REQUIRED)
find_package(Qt5Core REQUIRED)
find_package(Qt5DBus REQUIRED)

include(FindPkgConfig)
pkg_check_modules(GST_DEPS REQUIRED gstreamer-1.0 gstreamer-plugins-base-1.0 gstreamer-tag-1.0)
pkg_check_modules(GIO_DEPS REQUIRED gio-2.0 gio-unix-2.0)
pkg_check_modules(IMG_DEPS REQUIRED gdk-pixbuf-2.0 libexif)
pkg_check_modules(SOUP_DEPS REQUIRED libsoup-2.4)
pkg_check_modules(UNITY_API_DEPS REQUIRED libunity-api)
pkg_check_modules(XML_DEPS REQUIRED libxml-2.0)

include_directories(${GST_DEPS_INCLUDE_DIRS})
include_directories(${GIO_DEPS_INCLUDE_DIRS})
include_directories(${IMG_DEPS_INCLUDE_DIRS})
include_directories(${SOUP_DEPS_INCLUDE_DIRS})
include_directories(${XML_DEPS_INCLUDE_DIRS})
include_directories(include)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)

enable_testing()

add_subdirectory(src)
add_subdirectory(data)
add_subdirectory(plugins/Ubuntu/Thumbnailer)
add_subdirectory(tests)
add_subdirectory(tools)
add_subdirectory(include)
