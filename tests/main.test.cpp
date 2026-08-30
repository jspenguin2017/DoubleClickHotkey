#include "double_click_hotkey/platform_factory.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <initializer_list>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

int DoubleClickHotkeyMain(int argument_count, char* arguments[]);

namespace double_click_hotkey
{
namespace
{
struct MainPlatformState
{
    PlatformResult run_service_result;
    std::vector<WindowVisibility> sent_window_commands;
    std::vector<WindowVisibility> window_visibility_changes;
    std::vector<std::string> written_lines;
    int run_service_count = 0;
    int wait_for_key_count = 0;
};

MainPlatformState* active_platform_state = nullptr;
int platform_factory_call_count = 0;

class MainPlatformBinding final : public PlatformBinding
{
  public:
    explicit MainPlatformBinding(MainPlatformState& state) noexcept : state_(state)
    {
    }

    PlatformResult RunService(HotkeyEventHandler, WindowVisibilityHandler) override
    {
        ++state_.run_service_count;
        return state_.run_service_result;
    }

    PlatformResult ReserveSingleInstance() override
    {
        return {};
    }

    PlatformResult SendWindowCommand(const WindowVisibility visibility) override
    {
        state_.sent_window_commands.push_back(visibility);
        return {};
    }

    void SetWindowVisibility(const WindowVisibility visibility) override
    {
        state_.window_visibility_changes.push_back(visibility);
    }

    void WriteLine(const std::string_view message) override
    {
        state_.written_lines.emplace_back(message);
    }

    void WaitFor(std::chrono::milliseconds) override
    {
    }

    void WaitForKey() override
    {
        ++state_.wait_for_key_count;
    }

    PlatformResult SendF13() override
    {
        return {};
    }

    PlatformResult DoubleClick() override
    {
        return {};
    }

  private:
    MainPlatformState& state_;
};

int RunMain(const std::initializer_list<std::string_view> launch_arguments)
{
    std::vector<std::string> owned_arguments{"DoubleClickHotkey.exe"};
    owned_arguments.reserve(owned_arguments.size() + launch_arguments.size());
    for (const std::string_view argument : launch_arguments)
    {
        owned_arguments.emplace_back(argument);
    }

    std::vector<char*> arguments;
    arguments.reserve(owned_arguments.size());
    for (std::string& argument : owned_arguments)
    {
        arguments.push_back(argument.data());
    }

    return DoubleClickHotkeyMain(static_cast<int>(arguments.size()), arguments.data());
}

class MainTest : public testing::Test
{
  protected:
    void SetUp() override
    {
        active_platform_state = &platform_state;
        platform_factory_call_count = 0;
    }

    void TearDown() override
    {
        active_platform_state = nullptr;
    }

    MainPlatformState platform_state;
};
} // namespace

std::unique_ptr<PlatformBinding> CreatePlatformBinding()
{
    ++platform_factory_call_count;
    return std::make_unique<MainPlatformBinding>(*active_platform_state);
}

namespace
{
TEST_F(MainTest, RunsTheApplicationWithoutLaunchArguments)
{
    EXPECT_EQ(RunMain({}), 0);
    EXPECT_EQ(platform_factory_call_count, 1);
    EXPECT_EQ(platform_state.run_service_count, 1);
    EXPECT_EQ(platform_state.sent_window_commands.size(), 0U);
    EXPECT_EQ(platform_state.window_visibility_changes, (std::vector<WindowVisibility>{WindowVisibility::hidden}));
    EXPECT_EQ(platform_state.written_lines.size(), 0U);
}

TEST_F(MainTest, PassesALaunchArgumentToTheApplication)
{
    EXPECT_EQ(RunMain({"--show"}), 0);
    EXPECT_EQ(platform_factory_call_count, 1);
    EXPECT_EQ(platform_state.run_service_count, 0);
    EXPECT_EQ(platform_state.sent_window_commands, (std::vector<WindowVisibility>{WindowVisibility::shown}));
    EXPECT_EQ(platform_state.window_visibility_changes.size(), 0U);
    EXPECT_EQ(platform_state.written_lines.size(), 0U);
}

TEST_F(MainTest, StartsTheApplicationWithItsWindowShown)
{
    EXPECT_EQ(RunMain({"--start-shown"}), 0);
    EXPECT_EQ(platform_factory_call_count, 1);
    EXPECT_EQ(platform_state.run_service_count, 1);
    EXPECT_EQ(platform_state.sent_window_commands.size(), 0U);
    EXPECT_EQ(platform_state.window_visibility_changes, (std::vector<WindowVisibility>{WindowVisibility::shown}));
    EXPECT_EQ(platform_state.written_lines.size(), 0U);
}

TEST_F(MainTest, PassesEveryLaunchArgumentToTheParser)
{
    EXPECT_EQ(RunMain({"--show", "--hide"}), 1);
    EXPECT_EQ(platform_factory_call_count, 1);
    EXPECT_EQ(platform_state.run_service_count, 0);
    EXPECT_EQ(platform_state.sent_window_commands.size(), 0U);
    EXPECT_EQ(platform_state.window_visibility_changes, (std::vector<WindowVisibility>{WindowVisibility::shown}));
    EXPECT_EQ(platform_state.written_lines,
              (std::vector<std::string>{"Usage: DoubleClickHotkey [--start-shown | --show | --hide | --send-f13]"}));
    EXPECT_EQ(platform_state.wait_for_key_count, 0);
}

TEST_F(MainTest, ReturnsTheApplicationFailure)
{
    platform_state.run_service_result = {PlatformResultStatus::failure, "service failed"};

    EXPECT_EQ(RunMain({}), 1);
    EXPECT_EQ(platform_factory_call_count, 1);
    EXPECT_EQ(platform_state.run_service_count, 1);
    EXPECT_EQ(platform_state.window_visibility_changes,
              (std::vector<WindowVisibility>{WindowVisibility::hidden, WindowVisibility::shown}));
    EXPECT_EQ(platform_state.written_lines, (std::vector<std::string>{"service failed"}));
    EXPECT_EQ(platform_state.wait_for_key_count, 1);
}
} // namespace
} // namespace double_click_hotkey
