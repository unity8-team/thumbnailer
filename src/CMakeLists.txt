add_library(thumbnailer SHARED
audioimageextractor.cpp
imagescaler.cpp
thumbnailer.cpp
videoscreenshotter.cpp
)
target_link_libraries(thumbnailer ${DEPS_LDFLAGS})
