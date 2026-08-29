cmake_minimum_required(VERSION 3.28)

project(
    DoubleClickHotkey
    VERSION 1.0.0
    DESCRIPTION "Turn F8 into a global Windows double-click hotkey"
    LANGUAGES CXX
)

include(CTest)

option(DOUBLE_CLICK_HOTKEY_BUILD_APP "Build the Windows application" ${WIN32})

function(double_click_hotkey_enable_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

add_library(double_click_hotkey_core STATIC src/hotkey_action.cpp)
add_library(DoubleClickHotkey::core ALIAS double_click_hotkey_core)

target_compile_features(double_click_hotkey_core PUBLIC cxx_std_17)
target_include_directories(
    double_click_hotkey_core
    PUBLIC
        "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:include>"
)
set_target_properties(double_click_hotkey_core PROPERTIES CXX_EXTENSIONS OFF)
double_click_hotkey_enable_warnings(double_click_hotkey_core)

if(DOUBLE_CLICK_HOTKEY_BUILD_APP)
    if(NOT WIN32)
        message(FATAL_ERROR "The application target requires a Windows target toolchain")
    endif()

    add_executable(double_click_hotkey src/main.cpp)
    target_compile_features(double_click_hotkey PRIVATE cxx_std_17)
    target_compile_definitions(double_click_hotkey PRIVATE NOMINMAX UNICODE WIN32_LEAN_AND_MEAN _UNICODE)
    target_link_libraries(double_click_hotkey PRIVATE DoubleClickHotkey::core user32)
    set_target_properties(double_click_hotkey PROPERTIES CXX_EXTENSIONS OFF OUTPUT_NAME DoubleClickHotkey)
    double_click_hotkey_enable_warnings(double_click_hotkey)

    if(MINGW)
        target_link_options(double_click_hotkey PRIVATE -static -static-libgcc -static-libstdc++)
    endif()

    install(TARGETS double_click_hotkey RUNTIME DESTINATION bin)
endif()

set(
    DOUBLE_CLICK_HOTKEY_FORMAT_FILES
    "${PROJECT_SOURCE_DIR}/include/double_click_hotkey/hotkey_action.hpp"
    "${PROJECT_SOURCE_DIR}/src/hotkey_action.cpp"
    "${PROJECT_SOURCE_DIR}/src/main.cpp"
    "${PROJECT_SOURCE_DIR}/tests/hotkey_action_test.cpp"
)
include(cmake/ClangFormat.cmake)

if(BUILD_TESTING)
    include(FetchContent)

    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/063de7e9578f82b369302001269680b4b1553359.tar.gz
        URL_HASH SHA256=623fe05d283020f37a579a8985ba338091ac7229da62b5591ff40acdf05b95e2
        EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(googletest)

    add_subdirectory(tests)
endif()
