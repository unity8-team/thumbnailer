set(TESTDATADIR ${CMAKE_CURRENT_SOURCE_DIR})

configure_file(testsetup.h.in testsetup.h @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(basic basic.cpp)
target_link_libraries(basic thumbnailer)
add_test(basic basic)
set_property(TEST basic PROPERTY ENVIRONMENT "XDG_CACHE_HOME=${CMAKE_CURRENT_BINARY_DIR}")
set_property(TEST basic APPEND PROPERTY ENVIRONMENT "TN_UTILDIR=${CMAKE_BINARY_DIR}/src")
