add_subdirectory(core)

add_library(thumbnailer STATIC
audioimageextractor.cpp
file_io.cpp
image.cpp
lastfmdownloader.cpp
make_directories.cpp
safe_strerror.cpp
soupdownloader.cpp
thumbnailer.cpp
ubuntuserverdownloader.cpp
videoscreenshotter.cpp
)
qt5_use_modules(thumbnailer Core)

target_link_libraries(thumbnailer
 core
 ${Boost_LIBRARIES}
 ${GLIB_DEPS_LDFLAGS}
 ${GIO_DEPS_LDFLAGS}
 ${IMG_DEPS_LDFLAGS}
 ${XML_DEPS_LDFLAGS}
 ${SOUP_DEPS_LDFLAGS}
 #${UNITY_API_DEPS}    # Commented out because, currently, we only use ResourcePtr, which is header-only.
 )

add_subdirectory(service)
add_subdirectory(vs-thumb)

