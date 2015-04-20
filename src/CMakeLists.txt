add_subdirectory(core)

add_library(thumbnailer STATIC
audioimageextractor.cpp
imagescaler.cpp
lastfmdownloader.cpp
soupdownloader.cpp
thumbnailcache.cpp
thumbnailer.cpp
ubuntuserverdownloader.cpp
videoscreenshotter.cpp
safe_strerror.cpp
)

target_link_libraries(thumbnailer
 core
 ${GLIB_DEPS_LDFLAGS}
 ${GIO_DEPS_LDFLAGS}
 ${IMG_DEPS_LDFLAGS}
 ${XML_DEPS_LDFLAGS}
 ${SOUP_DEPS_LDFLAGS})
 #${UNITY_API_DEPS}    # Commented out because, currently, we only use ResourcePtr, which is header-only.

add_subdirectory(service)
add_subdirectory(vs-thumb)

