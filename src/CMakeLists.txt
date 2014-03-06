add_library(thumbnailer SHARED
#audioimageextractor.cpp
imagescaler.cpp
thumbnailcache.cpp
thumbnailer.cpp
videoscreenshotter.cpp
)


target_link_libraries(thumbnailer #${GST_DEPS_LDFLAGS}
 ${IMG_DEPS_LDFLAGS})
set_target_properties(thumbnailer PROPERTIES VERSION ${SO_VERSION} SOVERSION ${ABI_VERSION})
set_target_properties(thumbnailer PROPERTIES COMPILE_FLAGS "-DBUILDING_THUMBNAILER=1 -fvisibility=hidden")

install(
  TARGETS thumbnailer
  ARCHIVE DESTINATION ${LIBDIR}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION ${LIBDIR}
)

add_executable(vs-thumb vs-thumb.cpp)
target_link_libraries(vs-thumb ${GST_DEPS_LDFLAGS} ${IMG_DEPS_LDFLAGS})
install(
  TARGETS vs-thumb
  ARCHIVE DESTINATION ${SHARE_PRIV_DIR}
  RUNTIME DESTINATION ${SHARE_PRIV_DIR}
  LIBRARY DESTINATION ${SHARE_PRIV_DIR}
)
