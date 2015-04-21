project(thumbnailer C CXX)
cmake_minimum_required(VERSION 2.8.11)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# We require g++ 4.9, to avoid ABI breakage with earlier version.
# Note that this must match what is specifed in debian/control.
set(cxx_version_required 4.9)
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    if (NOT CMAKE_CXX_COMPILER_VERSION MATCHES "^${cxx_version_required}")
        message(FATAL_ERROR "g++ version must be ${cxx_version_required}, found ${CMAKE_CXX_COMPILER_VERSION}!")
    endif()
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -pedantic -Wno-variadic-macros -Wextra -fPIC")

# By default, all symbols are visible in the library. We strip out things we don't
# want at link time, by running a version script (see unity-scopes.map and the
# setting of LINK_FLAGS below).
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=default")

set(C_AND_CXX_WARNINGS "-pedantic -Wall -Wextra")

# Some additional warnings not included by the general flags set above.
set(EXTRA_C_WARNINGS "-Wcast-align -Wformat -Wredundant-decls -Wswitch-default")
set(EXTRA_CXX_WARNINGS "-Wnon-virtual-dtor -Wctor-dtor-privacy")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${C_AND_CXX_WARNINGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_C_WARNINGS}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 ${C_AND_CXX_WARNINGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EXTRA_C_WARNINGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EXTRA_CXX_WARNINGS}")

# -fno-permissive causes warnings with clang, so we only enable it for gcc
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-permissive")
endif()

if ("${cmake_build_type_lower}" STREQUAL "release" OR "${cmake_build_type_lower}" STREQUAL "relwithdebinfo")
    option(Werror "Treat warnings as errors" ON)
else()
    option(Werror "Treat warnings as errors" OFF)
endif()

# Some tests are slow, so make it possible not to run them
# during day-to-day development.
option(slowtests "Run slow tests" ON)

if (${Werror})
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
    if ("${cmake_build_type_lower}" STREQUAL "release" OR "${cmake_build_type_lower}" STREQUAL "relwithdebinfo")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-error=deprecated-declarations")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=deprecated-declarations")
    endif()
endif()

include(cmake/coverage.cmake)
include(cmake/UseGSettings.cmake)

include(GNUInstallDirs)
set(LIBDIR ${CMAKE_INSTALL_LIBDIR})
set(SHARE_PRIV_DIR ${CMAKE_INSTALL_LIBDIR}/thumbnailer)
set(SHARE_PRIV_ABS ${CMAKE_INSTALL_PREFIX}/${SHARE_PRIV_DIR})

find_package(Threads REQUIRED)
find_package(Qt5Core REQUIRED)
find_package(Qt5DBus REQUIRED)
find_package(Qt5Gui REQUIRED)
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
