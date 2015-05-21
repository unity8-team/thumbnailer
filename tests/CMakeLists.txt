add_subdirectory(utils)
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

# Make sure we always build gtest with debug info, otherwise we don't
# get line numbers for the code that is dragged in by the test macros.
set_property(TARGET gtest APPEND_STRING PROPERTY COMPILE_FLAGS " -g")

# gtest won't compile cleanly without this.
set_property(TARGET gtest APPEND_STRING PROPERTY COMPILE_FLAGS " -Wno-missing-field-initializers -Wno-old-style-cast")

set(TESTDATADIR ${CMAKE_CURRENT_SOURCE_DIR}/media)

configure_file(testsetup.h.in testsetup.h @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(file_io file_io.cpp)
target_link_libraries(file_io thumbnailer core gtest)
add_test(file_io file_io)

add_executable(image image.cpp)
target_link_libraries(image thumbnailer core gtest)
add_test(image image)

add_executable(safe_strerror_test safe_strerror_test.cpp)
target_link_libraries(safe_strerror_test thumbnailer core gtest)
add_test(safe_strerror_test safe_strerror_test)

add_executable(downloadtest download.cpp)

target_link_libraries(downloadtest thumbnailer testutils gtest)
add_test(downloadtest downloadtest)
set_property(TEST downloadtest PROPERTY ENVIRONMENT "GSETTINGS_SCHEMA_DIR=${CMAKE_BINARY_DIR}/data")
qt5_use_modules(downloadtest Core Network Test)


# This is only needed here because GStreamer brings in gobject.
pkg_check_modules(GOBJ_DEPS REQUIRED gobject-2.0)

include_directories(${GOBJ_DEPS_INCLUDE_DIRS})

add_executable(gobj_ptr gobj_ptr.cpp)
target_link_libraries(gobj_ptr ${GOBJ_DEPS_LDFLAGS} gtest)
add_test(gobj_ptr gobj_ptr)

add_executable(test_vsthumb
  test_vsthumb.cpp
  ${CMAKE_SOURCE_DIR}/src/vs-thumb/thumbnailextractor.cpp)
target_link_libraries(test_vsthumb
  ${GST_DEPS_LDFLAGS}
  ${IMG_DEPS_LDFLAGS}
  ${GIO_DEPS_LDFLAGS}
  gtest)
add_test(test_vsthumb test_vsthumb)

# Tests for the D-Bus service
pkg_check_modules(QTDBUSTEST_DEPS REQUIRED libqtdbustest-1)
add_definitions(${QTDBUSTEST_DEPS_CFLAGS})

set_source_files_properties(
  "${CMAKE_SOURCE_DIR}/src/service/dbusinterface.xml" PROPERTIES
  CLASSNAME ThumbnailerInterface)
qt5_add_dbus_interface(
  interface_files
  "${CMAKE_SOURCE_DIR}/src/service/dbusinterface.xml"
  thumbnailerinterface)

set_source_files_properties(
  "${CMAKE_SOURCE_DIR}/src/service/admininterface.xml" PROPERTIES
  CLASSNAME ThumbnailerAdminInterface
  INCLUDE ${CMAKE_SOURCE_DIR}/src/service/admininterface.h)
qt5_add_dbus_interface(
  admininterface_files
  "${CMAKE_SOURCE_DIR}/src/service/admininterface.xml"
  admininterface)

add_executable(test_dbus
  test_dbus.cpp
  ${CMAKE_SOURCE_DIR}/src/service/admininterface.cpp
  ${admininterface_files}
  ${interface_files})
qt5_use_modules(test_dbus DBus Test)
target_link_libraries(test_dbus
  thumbnailer
  ${QTDBUSTEST_DEPS_LDFLAGS}
  testutils
  gtest)
add_test(test_dbus test_dbus)
add_dependencies(test_dbus thumbnailer-service)

# Thumbnailer tests

add_executable(thumbnailer_test
    thumbnailer_test.cpp
    ${interface_files}
)
qt5_use_modules(thumbnailer_test DBus Test)
target_link_libraries(thumbnailer_test
    thumbnailer
    ${QTDBUSTEST_DEPS_LDFLAGS}
    testutils
    gtest)
add_test(thumbnailer_test thumbnailer_test)
add_dependencies(thumbnailer_test thumbnailer-service)

# Tests for the QML plugin
add_executable(test_qml test_qml.cpp)
qt5_use_modules(test_qml Qml DBus QuickTest)
target_link_libraries(test_qml
  ${QTDBUSTEST_DEPS_LDFLAGS} testutils)
add_test(test_qml xvfb-run -a -s "-screen 0 800x600x24" ./test_qml -import ${CMAKE_BINARY_DIR}/plugins)
add_dependencies(test_qml thumbnailer-service)
