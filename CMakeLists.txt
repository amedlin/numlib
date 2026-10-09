cmake_minimum_required(VERSION 3.20)

project(numlib
    VERSION 0.1.0
    DESCRIPTION "C++20 binary fixed-point arithmetic and exact integer square root"
    HOMEPAGE_URL "https://github.com/amedlin/numlib"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

add_library(numlib STATIC)
add_library(numlib::numlib ALIAS numlib)
target_sources(numlib PRIVATE
    numlib.cpp
    fixedpt/fixed.cpp
)
target_include_directories(numlib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_compile_features(numlib PUBLIC cxx_std_20)

# integerSqrt is inline in a header, so dependents must see the same ISA.
# MSVC /arch:AVX2 also enables BMI2 (lzcnt, shlx, shrx). Clang-cl sets MSVC.
if(MSVC)
    target_compile_options(numlib PUBLIC /arch:AVX2)
else()
    target_compile_options(numlib PUBLIC -mavx2)
endif()

install(TARGETS numlib
    EXPORT numlibTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(DIRECTORY fixedpt int
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)
install(EXPORT numlibTargets
    FILE numlibTargets.cmake
    NAMESPACE numlib::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/numlib
)

configure_package_config_file(
    cmake/numlibConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/numlibConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/numlib
)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/numlibConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/numlibConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/numlibConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/numlib
)

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(NUMLIB_BUILD_TESTS_DEFAULT ON)
else()
    set(NUMLIB_BUILD_TESTS_DEFAULT OFF)
endif()
option(NUMLIB_BUILD_TESTS
    "Build numlib tests and performance executable"
    ${NUMLIB_BUILD_TESTS_DEFAULT}
)

function(numlib_set_warnings target)
    target_compile_options(${target} PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra>
    )
endfunction()

numlib_set_warnings(numlib)

if(NUMLIB_BUILD_TESTS)
    include(FetchContent)

    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.16.0
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(Catch2)

    add_executable(test_isqrt testing/test_isqrt.cpp)
    target_link_libraries(test_isqrt PRIVATE numlib::numlib Catch2::Catch2WithMain)

    add_executable(test_fixed testing/test_fixed.cpp)
    target_link_libraries(test_fixed PRIVATE numlib::numlib Catch2::Catch2WithMain)

    add_executable(perf testing/perf.cpp)
    target_link_libraries(perf PRIVATE numlib::numlib)

    numlib_set_warnings(test_isqrt)
    numlib_set_warnings(test_fixed)
    numlib_set_warnings(perf)

    enable_testing()
    include(Catch)
    catch_discover_tests(test_isqrt TEST_SPEC "~[.exhaustive]")
    catch_discover_tests(test_fixed TEST_SPEC "~[.exhaustive]")
endif()
