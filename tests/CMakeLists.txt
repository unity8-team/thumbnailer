if (NOT DEFINED GTEST_ROOT)
    set(GTEST_ROOT /usr/src/gtest)
endif()

set(GTEST_SRC_DIR "${GTEST_ROOT}/src")
set(GTEST_INCLUDE_DIR ${GTEST_ROOT})

add_library(gtest STATIC
${GTEST_SRC_DIR}/gtest-all.cc
)
set_target_properties(gtest PROPERTIES INCLUDE_DIRECTORIES ${GTEST_INCLUDE_DIR})
target_link_libraries(gtest ${CMAKE_THREAD_LIBS_INIT})

set(TESTDATADIR ${CMAKE_CURRENT_SOURCE_DIR}/media)

configure_file(testsetup.h.in testsetup.h @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(basic basic.cpp)
target_link_libraries(basic thumbnailer gtest)
add_test(basic basic)
set_property(TEST basic PROPERTY ENVIRONMENT "XDG_CACHE_HOME=${CMAKE_CURRENT_BINARY_DIR}")
set_property(TEST basic APPEND PROPERTY ENVIRONMENT "TN_UTILDIR=${CMAKE_BINARY_DIR}/src/vs-thumb")

add_executable(mediaartcachetest mediaartcachetest.cpp ../src/mediaartcache.cpp)
target_link_libraries(mediaartcachetest ${GIO_DEPS_LDFLAGS} gtest)
add_test(mediaartcache mediaartcachetest)
set_property(TEST mediaartcache PROPERTY ENVIRONMENT "XDG_CACHE_HOME=${CMAKE_CURRENT_BINARY_DIR}")

add_executable(downloadtest download.cpp ../src/lastfmdownloader.cpp ../src/ubuntuserverdownloader.cpp ../src/soupdownloader.cpp)
target_link_libraries(downloadtest ${XML_DEPS_LDFLAGS} ${SOUP_DEPS_LDFLAGS} gtest)
add_test(downloadtest downloadtest)
set_property(TEST downloadtest PROPERTY ENVIRONMENT "GSETTINGS_SCHEMA_DIR=${CMAKE_BINARY_DIR}/data")

# This is only needed here because GStreamer brings in gobject.
pkg_check_modules(GOBJ_DEPS REQUIRED gobject-2.0)

include_directories(${GOBJ_DEPS_INCLUDE_DIRS})

add_executable(unique_gobj unique_gobj.cpp)
target_link_libraries(unique_gobj ${GOBJ_DEPS_LDFLAGS} ${IMG_DEPS_LDFLAGS} gtest)
add_test(unique_gobj unique_gobj)

add_executable(test_vsthumb
  test_vsthumb.cpp
  ../src/vs-thumb/thumbnailextractor.cpp)
target_link_libraries(test_vsthumb
  ${GST_DEPS_LDFLAGS}
  ${IMG_DEPS_LDFLAGS}
  ${GIO_DEPS_LDFLAGS}
  gtest)
add_test(test_vsthumb test_vsthumb)
