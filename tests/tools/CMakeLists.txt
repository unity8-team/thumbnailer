add_executable(thumbnailer-admin_test
    thumbnailer-admin_test.cpp
)
target_link_libraries(thumbnailer-admin_test
    gtest
    thumbnailer
    testutils
)
add_test(thumbnailer-admin_test thumbnailer-admin_test)
add_dependencies(thumbnailer-admin_test thumbnailer-admin thumbnailer-service)

include_directories(${CMAKE_SOURCE_DIR}/tests)
include_directories(${CMAKE_BINARY_DIR}/tests)
