add_executable(
    double_click_hotkey_tests
    application/application_test.cpp
    application/launch_command_test.cpp
    hotkey/hotkey_action_test.cpp
)
target_compile_features(double_click_hotkey_tests PRIVATE cxx_std_17)
target_link_libraries(double_click_hotkey_tests PRIVATE DoubleClickHotkey::core GTest::gtest_main)
set_target_properties(double_click_hotkey_tests PROPERTIES CXX_EXTENSIONS OFF)
double_click_hotkey_enable_warnings(double_click_hotkey_tests)

include(GoogleTest)
gtest_discover_tests(double_click_hotkey_tests DISCOVERY_MODE PRE_TEST)
