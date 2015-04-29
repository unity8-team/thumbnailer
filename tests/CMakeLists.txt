add_subdirectory(core)

if (NOT DEFINED GTEST_ROOT)
    set(GTEST_ROOT /usr/src/gtest)
endif()

set(GTEST_SRC_DIR "${GTEST_ROOT}/src")
set(GTEST_INCLUDE_DIR ${GTEST_ROOT})

add_library(gtest STATIC
${GTEST_SRC_DIR}/gtest-all.cc
${GTEST_SRC_DIR}/gtest_main.cc
)
set_target_properties(gtest PROPERTIES INCLUDE_DIRECTORIES ${GTEST_INCLUDE_DIR})
target_link_libraries(gtest ${CMAKE_THREAD_LIBS_INIT})

set(TESTDATADIR ${CMAKE_CURRENT_SOURCE_DIR}/media)

configure_file(testsetup.h.in testsetup.h @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(image image.cpp)
target_link_libraries(image thumbnailer core gtest)
add_test(image image)

add_executable(thumbnailer_test thumbnailer_test.cpp)
target_link_libraries(thumbnailer_test thumbnailer core gtest)
add_test(thumbnailer_test thumbnailer_test)

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

# Tests for the D-Bus service
pkg_check_modules(QTDBUSTEST_DEPS REQUIRED libqtdbustest-1)
add_definitions(${QTDBUSTEST_DEPS_CFLAGS})

add_executable(test_dbus
  test_dbus.cpp)
qt5_use_modules(test_dbus DBus Test)
target_link_libraries(test_dbus
  thumbnailer
  ${QTDBUSTEST_DEPS_LDFLAGS}
  ${IMG_DEPS_LDFLAGS}
  gtest)
add_test(test_dbus test_dbus)
add_dependencies(test_dbus thumbnailer-service)

# Tests for the QML plugin
add_executable(test_qml test_qml.cpp)
qt5_use_modules(test_qml Qml DBus QuickTest)
target_link_libraries(test_qml
  ${QTDBUSTEST_DEPS_LDFLAGS})
add_test(test_qml xvfb-run -a -s "-screen 0 800x600x24" ./test_qml -import ${CMAKE_BINARY_DIR}/plugins)
add_dependencies(test_qml thumbnailer-service)
