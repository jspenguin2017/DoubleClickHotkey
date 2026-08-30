add_library(double_click_hotkey_main_for_tests OBJECT "${PROJECT_SOURCE_DIR}/src/main.cpp")
target_compile_features(double_click_hotkey_main_for_tests PRIVATE cxx_std_17)
target_compile_definitions(double_click_hotkey_main_for_tests PRIVATE main=DoubleClickHotkeyMain)
target_link_libraries(double_click_hotkey_main_for_tests PRIVATE DoubleClickHotkey::core)
set_target_properties(double_click_hotkey_main_for_tests PROPERTIES CXX_EXTENSIONS OFF)
double_click_hotkey_enable_warnings(double_click_hotkey_main_for_tests)

add_executable(
    double_click_hotkey_tests
    $<TARGET_OBJECTS:double_click_hotkey_main_for_tests>
    application/application.test.cpp
    application/launch_command.test.cpp
    main.test.cpp
)
target_compile_features(double_click_hotkey_tests PRIVATE cxx_std_17)
target_link_libraries(double_click_hotkey_tests PRIVATE DoubleClickHotkey::core GTest::gtest_main)
set_target_properties(double_click_hotkey_tests PROPERTIES CXX_EXTENSIONS OFF)
double_click_hotkey_enable_warnings(double_click_hotkey_tests)

include(GoogleTest)
gtest_discover_tests(double_click_hotkey_tests DISCOVERY_MODE PRE_TEST)
