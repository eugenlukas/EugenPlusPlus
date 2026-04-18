file(GLOB_RECURSE LSP_SOURCES CONFIGURE_DEPENDS
    src/*.cpp
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/lsp)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/lsp/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/lsp/lib)

add_executable(lsp ${LSP_SOURCES})

target_include_directories(lsp PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(lsp PRIVATE core)