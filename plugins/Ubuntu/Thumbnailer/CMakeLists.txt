
set(QML_PLUGIN_DIR "${CMAKE_INSTALL_LIBDIR}/qt5/qml/Ubuntu/Thumbnailer.0.1")

include_directories(. ${CMAKE_CURRENT_BINARY_DIR})

set(dbusinterface_xml "${CMAKE_SOURCE_DIR}/src/service/dbusinterface.xml")
set_source_files_properties(${dbusinterface_xml} PROPERTIES
  CLASSNAME ThumbnailerInterface)
qt5_add_dbus_interface(interface_files ${dbusinterface_xml} thumbnailerinterface)

add_library(thumbnailer-qml MODULE
  plugin.cpp
  albumartgenerator.cpp
  artgeneratorcommon.cpp
  artistartgenerator.cpp
  thumbnailgenerator.cpp
  ${interface_files}
)

set_target_properties(thumbnailer-qml PROPERTIES AUTOMOC TRUE)
qt5_use_modules(thumbnailer-qml Qml Quick DBus)
target_link_libraries(thumbnailer-qml)

install(
  TARGETS thumbnailer-qml
  LIBRARY DESTINATION ${QML_PLUGIN_DIR}
)

install(
  FILES qmldir
  DESTINATION ${QML_PLUGIN_DIR}
)
