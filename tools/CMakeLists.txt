#add_executable(imdump imdump.cpp)
#target_link_libraries(imdump thumbnailer ${GST_DEPS_LDFLAGS})

add_executable(scaler scaler.cpp ../src/imagescaler.cpp)
target_link_libraries(scaler ${IMG_DEPS_LDFLAGS})

add_executable(cachetool cachetool.cpp)
target_link_libraries(cachetool thumbnailer)
