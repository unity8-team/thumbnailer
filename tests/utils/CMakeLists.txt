
include_directories("${CMAKE_BINARY_DIR}/tests")

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
  CLASSNAME AdminInterface
  INCLUDE ${CMAKE_SOURCE_DIR}/src/service/admininterface.h)
qt5_add_dbus_interface(
  interface_files
  "${CMAKE_SOURCE_DIR}/src/service/admininterface.xml"
  admininterface)

add_library(testutils STATIC
  artserver.cpp
  dbusserver.cpp
  ${interface_files}
  )
qt5_use_modules(testutils Core DBus)
