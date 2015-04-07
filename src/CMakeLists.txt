add_subdirectory(core)

add_library(thumbnailer SHARED
audioimageextractor.cpp
imagescaler.cpp
lastfmdownloader.cpp
soupdownloader.cpp
thumbnailcache.cpp
thumbnailer.cpp
ubuntuserverdownloader.cpp
videoscreenshotter.cpp
)

set(symbol_map "${CMAKE_CURRENT_SOURCE_DIR}/libthumbnailer.map")

target_link_libraries(thumbnailer
 core
 ${LEVELDB}
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

add_executable(vs-thumb vs-thumb.cpp)
target_link_libraries(vs-thumb core ${GST_DEPS_LDFLAGS} ${IMG_DEPS_LDFLAGS} ${GIO_DEPS_LDFLAGS})
install(
  TARGETS vs-thumb
  ARCHIVE DESTINATION ${SHARE_PRIV_DIR}
  RUNTIME DESTINATION ${SHARE_PRIV_DIR}
  LIBRARY DESTINATION ${SHARE_PRIV_DIR}
)

add_subdirectory(service)

