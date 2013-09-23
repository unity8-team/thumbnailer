add_library(thumbnailer SHARED
audioimageextractor.cpp
imagescaler.cpp
thumbnailcache.cpp
thumbnailer.cpp
videoscreenshotter.cpp
)
target_link_libraries(thumbnailer ${DEPS_LDFLAGS} ${IMG_DEPS_LDFLAGS})
