cmake_minimum_required(VERSION 3.5)
project(EugenPlusPlus)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optionen
option(BUILD_COMPILER "Build compiler executable" ON)
option(BUILD_LSP "Build language server" ON)

add_subdirectory(core)

if(BUILD_COMPILER)
    add_subdirectory(compiler)
endif()

if(BUILD_LSP)
    add_subdirectory(lsp)
endif()