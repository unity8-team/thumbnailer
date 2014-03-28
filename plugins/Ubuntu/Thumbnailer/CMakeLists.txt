
set(QML_PLUGIN_DIR "${CMAKE_INSTALL_LIBDIR}/qt5/qml/Ubuntu/Thumbnailer.0.1")

add_library(thumbnailer-qml MODULE
  plugin.cpp
  albumartgenerator.cpp
  thumbnailgenerator.cpp
)

set_target_properties(thumbnailer-qml PROPERTIES AUTOMOC TRUE)
qt5_use_modules(thumbnailer-qml Qml Quick DBus)
target_link_libraries(thumbnailer-qml thumbnailer)

install(
  TARGETS thumbnailer-qml
  LIBRARY DESTINATION ${QML_PLUGIN_DIR}
)

file(GLOB QMLFILES
  qmldir
)

install(
  FILES ${QMLFILES}
  DESTINATION ${QML_PLUGIN_DIR}
)
