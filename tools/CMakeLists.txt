# TODO: Re-enable  once ported.
#add_executable(imdump imdump.cpp)
#target_link_libraries(imdump thumbnailer ${GST_DEPS_LDFLAGS})

# TODO: Re-enable  once ported.
#add_executable(scaler scaler.cpp ../src/imagescaler.cpp)
#target_link_libraries(scaler ${IMG_DEPS_LDFLAGS})

add_executable(cachetool cachetool.cpp)
target_link_libraries(cachetool thumbnailer)

add_executable(downloaderstest downloaderstest.cpp)
target_link_libraries(downloaderstest thumbnailer)

qt5_use_modules(downloaderstest Core Network)
