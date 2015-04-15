set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_definitions(
  -fno-permissive
  -std=c++11
  -pedantic
  -Wall
  -Wextra
  -fPIC
)

add_subdirectory(core)

add_library(thumbnailer SHARED
audioimageextractor.cpp
imagescaler.cpp
lastfmdownloader.cpp
mediaartcache.cpp
soupdownloader.cpp
thumbnailcache.cpp
thumbnailer.cpp
ubuntuserverdownloader.cpp
videoscreenshotter.cpp
qurldownloader.cpp
${CMAKE_SOURCE_DIR}/include/internal/qartdownloader.h
${CMAKE_SOURCE_DIR}/include/internal/qurldownloader.h
${CMAKE_SOURCE_DIR}/include/internal/ubuntuserverdownloader.h
${CMAKE_SOURCE_DIR}/include/thumbnailer.h
)

find_package(Qt5Core REQUIRED)
find_package(Qt5Network REQUIRED)
find_package(Qt5Gui REQUIRED)

include_directories(${Qt5Core_INCLUDE_DIRS})
include_directories(${Qt5Network_INCLUDE_DIRS})
include_directories(${Qt5Gui_INCLUDE_DIRS})

qt5_use_modules(
    thumbnailer

    Core
    Network
    Gui
)

set(symbol_map "${CMAKE_CURRENT_SOURCE_DIR}/libthumbnailer.map")

target_link_libraries(thumbnailer
 core
 ${GLIB_DEPS_LDFLAGS}
 ${GIO_DEPS_LDFLAGS}
 ${IMG_DEPS_LDFLAGS}
 ${XML_DEPS_LDFLAGS}
 ${SOUP_DEPS_LDFLAGS})
set_target_properties(thumbnailer PROPERTIES VERSION ${SO_VERSION} SOVERSION ${ABI_VERSION})
set_target_properties(thumbnailer PROPERTIES
                      LINK_FLAGS "${ldflags} -Wl,--version-script,${symbol_map}")
set_target_properties(thumbnailer PROPERTIES LINK_DEPENDS ${symbol_map})

install(
  TARGETS thumbnailer
  ARCHIVE DESTINATION ${LIBDIR}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION ${LIBDIR}
)

add_subdirectory(service)
add_subdirectory(vs-thumb)

