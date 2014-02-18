set(TESTDATADIR ${CMAKE_CURRENT_SOURCE_DIR})

configure_file(testsetup.h.in testsetup.h @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(basic basic.cpp)
target_link_libraries(basic thumbnailer)
add_test(basic basic)
set_property(TEST basic PROPERTY ENVIRONMENT "XDG_CACHE_HOME=${CMAKE_CURRENT_BINARY_DIR}")
set_property(TEST basic APPEND PROPERTY ENVIRONMENT "TN_UTILDIR=${CMAKE_BINARY_DIR}/src")

# This is only needed here because GStreamer brings in gobject.
pkg_check_modules(GOBJ_DEPS REQUIRED gobject-2.0)

include_directories(${GOBJ_DEPS_INCLUDE_DIRS})

add_executable(unique_gobj unique_gobj.cpp)
target_link_libraries(unique_gobj ${GOBJ_DEPS_LDFLAGS} ${IMG_DEPS_LDFLAGS})
add_test(unique_gobj unique_gobj)
