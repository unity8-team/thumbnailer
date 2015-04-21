
#add_executable(imdump imdump.cpp)
#target_link_libraries(imdump thumbnailer ${GST_DEPS_LDFLAGS})

add_executable(scaler scaler.cpp)
target_link_libraries(scaler thumbnailer ${IMG_DEPS_LDFLAGS})

add_executable(cachetool cachetool.cpp)
target_link_libraries(cachetool thumbnailer)

add_executable(lastfmtest lastfmtest.cpp)
target_link_libraries(lastfmtest thumbnailer ${SOUP_DEPS_LDFLAGS} ${XML_DEPS_LDFLAGS})

add_executable(ubuntuservertest ubuntuservertest.cpp)
target_link_libraries(ubuntuservertest thumbnailer ${SOUP_DEPS_LDFLAGS})

qt5_use_modules(ubuntuservertest Core Network)
