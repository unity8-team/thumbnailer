add_executable(imdump imdump.cpp)
target_link_libraries(imdump thumbnailer)

add_executable(videodump videodump.cpp)
target_link_libraries(videodump thumbnailer)
