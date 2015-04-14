#add_executable(imdump imdump.cpp)
#target_link_libraries(imdump thumbnailer ${GST_DEPS_LDFLAGS})

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_definitions(
  -fno-permissive
  -std=c++11
  -pedantic
  -Wall
  -Wextra
  -fPIC
)

find_package(Qt5Core REQUIRED)
find_package(Qt5Network REQUIRED)
find_package(Qt5Gui REQUIRED)

include_directories(${Qt5Core_INCLUDE_DIRS})
include_directories(${Qt5Network_INCLUDE_DIRS})
include_directories(${Qt5Gui_INCLUDE_DIRS})

add_executable(scaler scaler.cpp ../src/imagescaler.cpp)
target_link_libraries(scaler ${IMG_DEPS_LDFLAGS})

add_executable(cachetool cachetool.cpp)
target_link_libraries(cachetool thumbnailer)

add_executable(lastfmtest lastfmtest.cpp ../src/lastfmdownloader.cpp ../src/soupdownloader.cpp)
target_link_libraries(lastfmtest ${SOUP_DEPS_LDFLAGS} ${XML_DEPS_LDFLAGS})

add_executable(ubuntuservertest
    ubuntuservertest.cpp
    ../src/ubuntuserverdownloader.cpp
    ../src/soupdownloader.cpp
    ../src/qartdownloader.cpp
    ${CMAKE_SOURCE_DIR}/include/internal/qartdownloader.h
    ${CMAKE_SOURCE_DIR}/include/internal/ubuntuserverdownloader.h
    )
target_link_libraries(ubuntuservertest ${SOUP_DEPS_LDFLAGS})

qt5_use_modules(
    ubuntuservertest

    Core
    Network
    Gui
)
