file(GLOB_RECURSE LSP_SOURCES CONFIGURE_DEPENDS
    src/*.cpp
)

add_library(lsp ${LSP_SOURCES})

target_include_directories(lsp PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)


target_link_libraries(lsp PRIVATE core)