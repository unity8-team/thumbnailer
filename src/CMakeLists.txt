add_subdirectory(core)

add_library(thumbnailer STATIC
audioimageextractor.cpp
imagescaler.cpp
lastfmdownloader.cpp
mediaartcache.cpp
soupdownloader.cpp
thumbnailcache.cpp
thumbnailer.cpp
ubuntuserverdownloader.cpp
videoscreenshotter.cpp
)

target_link_libraries(thumbnailer
 core
 ${GLIB_DEPS_LDFLAGS}
 ${GIO_DEPS_LDFLAGS}
 ${IMG_DEPS_LDFLAGS}
 ${XML_DEPS_LDFLAGS}
 ${SOUP_DEPS_LDFLAGS})

add_subdirectory(service)
add_subdirectory(vs-thumb)

