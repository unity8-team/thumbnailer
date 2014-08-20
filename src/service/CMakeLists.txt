add_definitions(${THUMBNAILER_CFLAGS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(thumbnailer-service
  main.cpp
  dbusinterface.cpp
  dbus-generated.c
)

target_link_libraries(thumbnailer-service thumbnailer ${CMAKE_THREAD_LIBS_INIT})

install(
  TARGETS thumbnailer-service
  RUNTIME DESTINATION ${SHARE_PRIV_DIR}
)

find_program(gdbus_codegen gdbus-codegen)
if(NOT gdbus_codegen)
  msg(FATAL_ERROR "Could not locate gdbus-codegen")
endif()

add_custom_command(
  OUTPUT dbus-generated.c dbus-generated.h
  COMMAND ${gdbus_codegen} --interface-prefix=com.canonical. --generate-c-code dbus-generated --c-namespace TN ${CMAKE_CURRENT_SOURCE_DIR}/dbus-interface.xml
  MAIN_DEPENDENCY dbus-interface.xml
)

# Install the service file.
configure_file(com.canonical.Thumbnailer.service.in com.canonical.Thumbnailer.service)

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/com.canonical.Thumbnailer.service
  DESTINATION ${CMAKE_INSTALL_DATADIR}/dbus-1/services
)
