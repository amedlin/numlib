cmake_minimum_required(VERSION 3.20)

project(numlib LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.16.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(Catch2)

add_library(numlib STATIC)
target_sources(numlib PRIVATE
    numlib.cpp
    fixedpt/fixed.cpp
)
target_include_directories(numlib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# integerSqrt is inline in a header, so dependents must see the same ISA.
# MSVC /arch:AVX2 also enables BMI2 (lzcnt, shlx, shrx). Clang-cl sets MSVC.
if(MSVC)
    target_compile_options(numlib PUBLIC /arch:AVX2)
else()
    target_compile_options(numlib PUBLIC -mavx2)
endif()

add_executable(test_isqrt testing/test_isqrt.cpp)
target_link_libraries(test_isqrt PRIVATE numlib Catch2::Catch2WithMain)

add_executable(test_fixed testing/test_fixed.cpp)
target_link_libraries(test_fixed PRIVATE numlib Catch2::Catch2WithMain)

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
numlib_set_warnings(test_fixed)
numlib_set_warnings(perf)

enable_testing()
include(Catch)
catch_discover_tests(test_isqrt TEST_SPEC "~[.exhaustive]")
catch_discover_tests(test_fixed TEST_SPEC "~[.exhaustive]")
