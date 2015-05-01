add_subdirectory(core)

add_library(thumbnailer STATIC
audioimageextractor.cpp
file_io.cpp
image.cpp
lastfmdownloader.cpp
make_directories.cpp
safe_strerror.cpp
thumbnailer.cpp
ubuntuserverdownloader.cpp
videoscreenshotter.cpp
artdownloader.cpp
syncdownloader.cpp
${CMAKE_SOURCE_DIR}/include/internal/artdownloader.h
${CMAKE_SOURCE_DIR}/include/internal/ubuntuserverdownloader.h
${CMAKE_SOURCE_DIR}/include/internal/lastfmdownloader.h
${CMAKE_SOURCE_DIR}/include/internal/artreply.h
${CMAKE_SOURCE_DIR}/include/internal/syncdownloader.h
${CMAKE_SOURCE_DIR}/include/thumbnailer.h
)
qt5_use_modules(thumbnailer Core)

qt5_use_modules(thumbnailer Core Network)
set_target_properties(thumbnailer PROPERTIES AUTOMOC TRUE)

target_link_libraries(thumbnailer
 core
 ${Boost_LIBRARIES}
 ${GLIB_DEPS_LDFLAGS}
 ${GIO_DEPS_LDFLAGS}
 ${IMG_DEPS_LDFLAGS}
 #${UNITY_API_DEPS}    # Commented out because, currently, we only use ResourcePtr, which is header-only.
 )

add_subdirectory(service)
add_subdirectory(vs-thumb)

