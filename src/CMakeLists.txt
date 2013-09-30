add_library(thumbnailer SHARED
audioimageextractor.cpp
imagescaler.cpp
thumbnailcache.cpp
thumbnailer.cpp
videoscreenshotter.cpp
)
target_link_libraries(thumbnailer ${GST_DEPS_LDFLAGS} ${IMG_DEPS_LDFLAGS})
set_target_properties(thumbnailer PROPERTIES VERSION ${SO_VERSION} SOVERSION ${ABI_VERSION})

install(
  TARGETS thumbnailer
  ARCHIVE DESTINATION ${LIBDIR}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION ${LIBDIR}
)
