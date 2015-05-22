
include_directories("${CMAKE_BINARY_DIR}/tests")

add_library(testutils STATIC
  artserver.cpp
  )
qt5_use_modules(testutils Core)
