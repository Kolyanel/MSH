cmake_minimum_required(VERSION 3.16)

project(msh C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

# =========================
# BUILD TYPE
# =========================
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_C_FLAGS_DEBUG "-g -O0 -Wall -Wextra -Werror -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")

# =========================
# PATHS
# =========================
set(PROJECT_ROOT ${CMAKE_SOURCE_DIR})

set(MSH_DIR   ${PROJECT_ROOT}/msh)
set(MYLIB_DIR ${PROJECT_ROOT}/mylib)
set(MIMALLOC_DIR /root/mimalloc)

# =========================
# OUTPUT
# =========================
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY /root/Coding_C)

# =========================
# MYLIB
# =========================
add_library(mylib STATIC
    ${MYLIB_DIR}/src/buf.c
    ${MYLIB_DIR}/src/io.c
    ${MYLIB_DIR}/src/memory.c
    ${MYLIB_DIR}/src/my_string.c
    ${MYLIB_DIR}/src/utils.c
    ${MYLIB_DIR}/src/vector.c
    ${MYLIB_DIR}/src/utf8.c
)

target_include_directories(mylib PUBLIC
    ${MYLIB_DIR}/include
)

# =========================
# mimalloc
# =========================
set(MI_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(MI_BUILD_STATIC ON CACHE BOOL "" FORCE)
set(MI_BUILD_TESTS OFF CACHE BOOL "" FORCE)

add_subdirectory(
    ${MIMALLOC_DIR}
    ${CMAKE_BINARY_DIR}/mimalloc-build
)

# =========================
# MSH SOURCES
# =========================
file(GLOB_RECURSE MSH_SOURCES CONFIGURE_DEPENDS
    ${MSH_DIR}/src/*.c
)

add_executable(msh ${MSH_SOURCES})

# =========================
# INCLUDE DIRS
# =========================
target_include_directories(msh PRIVATE
    ${MSH_DIR}/include
    ${MSH_DIR}/src
    ${MSH_DIR}/src/ast
    ${MSH_DIR}/src/core
    ${MSH_DIR}/src/environ
    ${MSH_DIR}/src/executor
    ${MSH_DIR}/src/executor/builtin
    ${MSH_DIR}/src/executor/context
    ${MSH_DIR}/src/executor/exec_argv
    ${MSH_DIR}/src/executor/exec_ast
    ${MSH_DIR}/src/executor/expand
    ${MSH_DIR}/src/executor/job
    ${MSH_DIR}/src/executor/pipe
    ${MSH_DIR}/src/executor/pipeline
    ${MSH_DIR}/src/executor/redir
    ${MSH_DIR}/src/executor/redir/heredoc
    ${MSH_DIR}/src/executor/signal
    ${MSH_DIR}/src/executor/state
    ${MSH_DIR}/src/executor/wait
    ${MSH_DIR}/src/lexer
    ${MSH_DIR}/src/parser
    ${MSH_DIR}/src/path
    ${MSH_DIR}/src/utils
    ${MYLIB_DIR}/include
)

# =========================
# LINK
# =========================
target_link_libraries(msh PRIVATE
    mylib
    mimalloc-static
    pthread
)