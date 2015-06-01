set(TESTDATADIR ${CMAKE_CURRENT_SOURCE_DIR}/media)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

configure_file(testsetup.h.in testsetup.h @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(dbus)
add_subdirectory(download)
add_subdirectory(file_io)
add_subdirectory(gobj_ptr)
add_subdirectory(image)
add_subdirectory(qml)
add_subdirectory(safe_strerror)
add_subdirectory(settings)
add_subdirectory(thumbnailer)
add_subdirectory(tools)
add_subdirectory(utils)
add_subdirectory(vsthumb)

add_subdirectory(copyright)
add_subdirectory(headers)
add_subdirectory(whitespace)

add_subdirectory(core)

if (NOT DEFINED GTEST_ROOT)
    set(GTEST_ROOT /usr/src/gtest)
endif()

set(GTEST_SRC_DIR "${GTEST_ROOT}/src")
set(GTEST_INCLUDE_DIR ${GTEST_ROOT})

add_library(gtest STATIC
    ${GTEST_SRC_DIR}/gtest-all.cc
    ${GTEST_SRC_DIR}/gtest_main.cc
)
set_target_properties(gtest PROPERTIES INCLUDE_DIRECTORIES ${GTEST_INCLUDE_DIR})
target_link_libraries(gtest ${CMAKE_THREAD_LIBS_INIT})

# Make sure we always build gtest with debug info, otherwise we don't
# get line numbers for the code that is dragged in by the test macros.
set_property(TARGET gtest APPEND_STRING PROPERTY COMPILE_FLAGS " -g")

# gtest won't compile cleanly without this.
set_property(TARGET gtest APPEND_STRING PROPERTY COMPILE_FLAGS " -Wno-missing-field-initializers -Wno-old-style-cast")
