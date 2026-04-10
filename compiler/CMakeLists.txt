file(GLOB_RECURSE COMPILER_SOURCES CONFIGURE_DEPENDS
    src/*.cpp
)

add_library(compiler ${COMPILER_SOURCES})

target_include_directories(compiler PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(compiler PRIVATE core)