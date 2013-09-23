add_executable(basic basic.cpp)
target_link_libraries(basic thumbnailer)
add_test(basic basic)
set_tests_properties(basic PROPERTIES ENVIRONMENT "XDG_CACHE_HOME=${CMAKE_CURRENT_BINARY_DIR}")
