cmake_minimum_required(VERSION 3.10)
project(EugenPlusPlus)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Options
option(DEBUG_SYMBOLS "include debug symbols" ON)
option(BUILD_COMPILER "Build compiler executable" ON)
option(BUILD_LSP "Build language server" ON)

if(DEBUG_SYMBOLS)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# fmt
include(FetchContent)
FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt
  GIT_TAG        e69e5f977d458f2650bb346dadf2ad30c5320281) # 10.2.1
FetchContent_MakeAvailable(fmt)

# add core dir
add_subdirectory(core)

# add compiler dir
if(BUILD_COMPILER)
    add_subdirectory(compiler)
endif()

# add lsp dir
if(BUILD_LSP)
    add_subdirectory(lsp)
endif()
