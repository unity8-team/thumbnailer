
set(QML_PLUGIN_DIR "${CMAKE_INSTALL_LIBDIR}/qt5/qml/Ubuntu/Thumbnailer.0.1")

add_library(thumbnailer-qml MODULE
  plugin.cpp
  albumartgenerator.cpp
  artgeneratorcommon.cpp
  artistartgenerator.cpp
  thumbnailgenerator.cpp
  qthumbnailer.cpp
)

set_target_properties(thumbnailer-qml PROPERTIES AUTOMOC TRUE)
qt5_use_modules(thumbnailer-qml Qml Quick DBus)
target_link_libraries(thumbnailer-qml thumbnailer)

install(
  TARGETS thumbnailer-qml
  LIBRARY DESTINATION ${QML_PLUGIN_DIR}
)

install(
  FILES qmldir
  DESTINATION ${QML_PLUGIN_DIR}
)
