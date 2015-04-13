find_library(LEVELDB NAMES leveldb REQUIRED)

add_subdirectory(internal)

set(CACHE_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/persistent_cache_stats.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/persistent_string_cache.cpp
)

set(CORE_SRC ${CORE_SRC} ${CACHE_SRC})

add_library(core STATIC ${CORE_SRC})
set_target_properties(core PROPERTIES COMPILE_FLAGS "-fPIC")
target_link_libraries(core ${LEVELDB})
