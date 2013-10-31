#add_executable(imdump imdump.cpp)
#target_link_libraries(imdump thumbnailer ${GST_DEPS_LDFLAGS})

add_executable(scaler scaler.cpp)
target_link_libraries(scaler thumbnailer)

add_executable(cachetool cachetool.cpp)
target_link_libraries(cachetool thumbnailer)

include_directories(${SOUP_DEPS_INCLUDE_DIRS})
add_executable(lastfmtest lastfmtest.cpp)
target_link_libraries(lastfmtest ${SOUP_DEPS_LDFLAGS})
