set(TESTDATADIR ${CMAKE_CURRENT_SOURCE_DIR})

configure_file(testsetup.h.in testsetup.h @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(basic basic.cpp)
target_link_libraries(basic thumbnailer)
add_test(basic basic)
set_tests_properties(basic PROPERTIES ENVIRONMENT "XDG_CACHE_HOME=${CMAKE_CURRENT_BINARY_DIR}")
