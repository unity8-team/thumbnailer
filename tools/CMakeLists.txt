add_executable(imdump imdump.cpp)
target_link_libraries(imdump thumbnailer)

add_executable(videodump videodump.cpp)
target_link_libraries(videodump thumbnailer)

add_executable(scaler scaler.cpp)
target_link_libraries(scaler thumbnailer)

add_executable(cachetool cachetool.cpp)
target_link_libraries(cachetool thumbnailer)
