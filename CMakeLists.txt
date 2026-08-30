cmake_minimum_required(VERSION 3.28)

project(
    DoubleClickHotkey
    VERSION 1.0.0
    DESCRIPTION "Turn F13 into a global Windows double-click hotkey"
    LANGUAGES CXX
)

include(CTest)

option(DOUBLE_CLICK_HOTKEY_BUILD_APP "Build the application for a supported platform" ${WIN32})

function(double_click_hotkey_enable_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

add_library(
    double_click_hotkey_core
    STATIC
        src/application/application.cpp
        src/application/launch_command.cpp
)
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
    if(WIN32)
        set(
            DOUBLE_CLICK_HOTKEY_PLATFORM_SOURCES
            src/platform/windows/console.cpp
            src/platform/windows/console_control_handler.cpp
            src/platform/windows/input_injector.cpp
            src/platform/windows/instance_command.cpp
            src/platform/windows/keyboard_hook.cpp
            src/platform/windows/keyboard_sender.cpp
            src/platform/windows/mouse.cpp
            src/platform/windows/platform_factory.cpp
            src/platform/windows/single_instance.cpp
            src/platform/windows/windows_platform_binding.cpp
        )
        set(DOUBLE_CLICK_HOTKEY_PLATFORM_LIBRARIES user32)
    else()
        message(FATAL_ERROR "No application platform binding is available for ${CMAKE_SYSTEM_NAME}")
    endif()

    add_library(double_click_hotkey_platform STATIC ${DOUBLE_CLICK_HOTKEY_PLATFORM_SOURCES})
    add_library(DoubleClickHotkey::platform ALIAS double_click_hotkey_platform)
    target_compile_features(double_click_hotkey_platform PUBLIC cxx_std_17)
    target_compile_definitions(double_click_hotkey_platform PRIVATE NOMINMAX UNICODE WIN32_LEAN_AND_MEAN _UNICODE)
    target_include_directories(double_click_hotkey_platform PRIVATE "${PROJECT_SOURCE_DIR}/src")
    target_link_libraries(
        double_click_hotkey_platform
        PUBLIC DoubleClickHotkey::core
        PRIVATE ${DOUBLE_CLICK_HOTKEY_PLATFORM_LIBRARIES}
    )
    set_target_properties(double_click_hotkey_platform PROPERTIES CXX_EXTENSIONS OFF)
    double_click_hotkey_enable_warnings(double_click_hotkey_platform)

    add_executable(double_click_hotkey src/main.cpp)
    target_compile_features(double_click_hotkey PRIVATE cxx_std_17)
    target_link_libraries(double_click_hotkey PRIVATE DoubleClickHotkey::platform)
    set_target_properties(double_click_hotkey PROPERTIES CXX_EXTENSIONS OFF OUTPUT_NAME DoubleClickHotkey)
    double_click_hotkey_enable_warnings(double_click_hotkey)

    if(MINGW)
        target_link_options(double_click_hotkey PRIVATE -static -static-libgcc -static-libstdc++)
    endif()

    install(TARGETS double_click_hotkey RUNTIME DESTINATION bin)
endif()

set(
    DOUBLE_CLICK_HOTKEY_FORMAT_FILES
    "${PROJECT_SOURCE_DIR}/include/double_click_hotkey/application.hpp"
    "${PROJECT_SOURCE_DIR}/include/double_click_hotkey/launch_command.hpp"
    "${PROJECT_SOURCE_DIR}/include/double_click_hotkey/platform_binding.hpp"
    "${PROJECT_SOURCE_DIR}/include/double_click_hotkey/platform_factory.hpp"
    "${PROJECT_SOURCE_DIR}/src/application/application.cpp"
    "${PROJECT_SOURCE_DIR}/src/application/launch_command.cpp"
    "${PROJECT_SOURCE_DIR}/src/main.cpp"
    "${PROJECT_SOURCE_DIR}/src/platform/windows/console.cpp"
    "${PROJECT_SOURCE_DIR}/src/platform/windows/console.hpp"
    "${PROJECT_SOURCE_DIR}/src/platform/windows/console_control_handler.cpp"
    "${PROJECT_SOURCE_DIR}/src/platform/windows/console_control_handler.hpp"
    "${PROJECT_SOURCE_DIR}/src/platform/windows/input_injector.cpp"
    "${PROJECT_SOURCE_DIR}/src/platform/windows/input_injector.hpp"
    "${PROJECT_SOURCE_DIR}/src/platform/windows/instance_command.cpp"
    "${PROJECT_SOURCE_DIR}/src/platform/windows/instance_command.hpp"
    "${PROJECT_SOURCE_DIR}/src/platform/windows/keyboard_hook.cpp"
    "${PROJECT_SOURCE_DIR}/src/platform/windows/keyboard_hook.hpp"
    "${PROJECT_SOURCE_DIR}/src/platform/windows/keyboard_sender.cpp"
    "${PROJECT_SOURCE_DIR}/src/platform/windows/keyboard_sender.hpp"
    "${PROJECT_SOURCE_DIR}/src/platform/windows/mouse.cpp"
    "${PROJECT_SOURCE_DIR}/src/platform/windows/mouse.hpp"
    "${PROJECT_SOURCE_DIR}/src/platform/windows/platform_factory.cpp"
    "${PROJECT_SOURCE_DIR}/src/platform/windows/single_instance.cpp"
    "${PROJECT_SOURCE_DIR}/src/platform/windows/single_instance.hpp"
    "${PROJECT_SOURCE_DIR}/src/platform/windows/windows_platform_binding.cpp"
    "${PROJECT_SOURCE_DIR}/src/platform/windows/windows_platform_binding.hpp"
    "${PROJECT_SOURCE_DIR}/tests/application/application.test.cpp"
    "${PROJECT_SOURCE_DIR}/tests/application/launch_command.test.cpp"
    "${PROJECT_SOURCE_DIR}/tests/main.test.cpp"
    "${PROJECT_SOURCE_DIR}/tests/platform/windows/input_senders.test.cpp"
    "${PROJECT_SOURCE_DIR}/tests/platform/windows/instance_command.test.cpp"
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
