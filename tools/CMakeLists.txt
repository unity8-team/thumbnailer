#add_executable(imdump imdump.cpp)
#target_link_libraries(imdump thumbnailer ${GST_DEPS_LDFLAGS})

#add_executable(scaler scaler.cpp ../src/imagescaler.cpp)
#target_link_libraries(scaler ${IMG_DEPS_LDFLAGS})

add_executable(cachetool cachetool.cpp)
target_link_libraries(cachetool thumbnailer)

add_executable(lastfmtest lastfmtest.cpp ../src/lastfmdownloader.cpp ../src/soupdownloader.cpp)
target_link_libraries(lastfmtest ${SOUP_DEPS_LDFLAGS} ${XML_DEPS_LDFLAGS})

add_executable(ubuntuservertest ubuntuservertest.cpp ../src/ubuntuserverdownloader.cpp ../src/soupdownloader.cpp)
target_link_libraries(ubuntuservertest ${SOUP_DEPS_LDFLAGS})
