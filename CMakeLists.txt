cmake_minimum_required(VERSION 3.20)

project(numlib LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(numlib STATIC)
target_sources(numlib PRIVATE
    numlib.cpp
)
target_include_directories(numlib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(test_isqrt testing/test_isqrt.cpp)
target_link_libraries(test_isqrt PRIVATE numlib)

add_executable(perf testing/perf.cpp)
target_link_libraries(perf PRIVATE numlib)

function(numlib_set_warnings target)
    target_compile_options(${target} PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra>
    )
endfunction()

numlib_set_warnings(numlib)
numlib_set_warnings(test_isqrt)
numlib_set_warnings(perf)
