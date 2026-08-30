#include "double_click_hotkey/application.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <initializer_list>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace double_click_hotkey
{
namespace
{
class FakePlatformBinding final : public PlatformBinding
{
  public:
    PlatformResult RunService(HotkeyEventHandler hotkey_handler, WindowVisibilityHandler visibility_handler) override
    {
        operations.emplace_back("run service");
        ++run_service_count;
        for (const HotkeyEvent& event : hotkey_events)
        {
            hotkey_handler(event);
        }
        for (const WindowVisibility visibility : received_window_commands)
        {
            visibility_handler(visibility);
        }
        return run_service_result;
    }

    PlatformResult ReserveSingleInstance() override
    {
        operations.emplace_back("reserve instance");
        ++reserve_single_instance_count;
        return reserve_single_instance_result;
    }

    PlatformResult SendWindowCommand(const WindowVisibility visibility) override
    {
        operations.emplace_back("send window command");
        sent_window_commands.push_back(visibility);
        return send_window_command_result;
    }

    void SetWindowVisibility(const WindowVisibility visibility) override
    {
        operations.emplace_back("set window visibility");
        window_visibility_changes.push_back(visibility);
    }

    void WriteLine(const std::string_view message) override
    {
        operations.emplace_back("write line");
        written_lines.emplace_back(message);
    }

    void WaitFor(const std::chrono::milliseconds duration) override
    {
        operations.emplace_back("wait");
        waits.push_back(duration);
    }

    void WaitForKey() override
    {
        operations.emplace_back("wait for key");
        ++wait_for_key_count;
    }

    PlatformResult SendF13() override
    {
        operations.emplace_back("send F13");
        ++send_f13_count;
        return send_f13_result;
    }

    PlatformResult DoubleClick() override
    {
        operations.emplace_back("double click");
        ++double_click_count;
        return double_click_result;
    }

    PlatformResult run_service_result;
    PlatformResult reserve_single_instance_result;
    PlatformResult send_window_command_result;
    PlatformResult send_f13_result;
    PlatformResult double_click_result;
    std::vector<HotkeyEvent> hotkey_events;
    std::vector<WindowVisibility> received_window_commands;
    std::vector<WindowVisibility> sent_window_commands;
    std::vector<WindowVisibility> window_visibility_changes;
    std::vector<std::string> written_lines;
    std::vector<std::chrono::milliseconds> waits;
    std::vector<std::string> operations;
    int run_service_count = 0;
    int reserve_single_instance_count = 0;
    int send_f13_count = 0;
    int double_click_count = 0;
    int wait_for_key_count = 0;
};

void ExpectErrorReported(const FakePlatformBinding& platform,
                         const std::initializer_list<std::string_view> expected_lines,
                         const std::initializer_list<WindowVisibility> expected_visibility_changes,
                         const std::initializer_list<std::string_view> expected_operations, const bool waited_for_key)
{
    std::vector<std::string> lines;
    lines.reserve(expected_lines.size());
    for (const std::string_view line : expected_lines)
    {
        lines.emplace_back(line);
    }

    const std::vector<WindowVisibility> visibility_changes(expected_visibility_changes);

    std::vector<std::string> operations;
    operations.reserve(expected_operations.size());
    for (const std::string_view operation : expected_operations)
    {
        operations.emplace_back(operation);
    }

    EXPECT_EQ(platform.written_lines, lines);
    EXPECT_EQ(platform.window_visibility_changes, visibility_changes);
    EXPECT_EQ(platform.operations, operations);
    EXPECT_EQ(platform.wait_for_key_count, waited_for_key ? 1 : 0);
}

TEST(ApplicationTest, RunsTheServiceAndHidesItsWindow)
{
    FakePlatformBinding platform;
    Application application(platform);

    EXPECT_EQ(application.Run(), 0);
    EXPECT_EQ(platform.run_service_count, 1);
    EXPECT_EQ(platform.window_visibility_changes, (std::vector<WindowVisibility>{WindowVisibility::hidden}));
    EXPECT_EQ(platform.written_lines.size(), 0U);
    EXPECT_EQ(platform.wait_for_key_count, 0);
    EXPECT_EQ(platform.operations, (std::vector<std::string>{"set window visibility", "run service"}));
}

TEST(ApplicationTest, RunsTheServiceWithItsWindowShownWhenRequested)
{
    FakePlatformBinding platform;
    Application application(platform, LaunchCommand::start_shown);

    EXPECT_EQ(application.Run(), 0);
    EXPECT_EQ(platform.run_service_count, 1);
    EXPECT_EQ(platform.window_visibility_changes, (std::vector<WindowVisibility>{WindowVisibility::shown}));
    EXPECT_EQ(platform.written_lines.size(), 0U);
    EXPECT_EQ(platform.wait_for_key_count, 0);
    EXPECT_EQ(platform.operations, (std::vector<std::string>{"set window visibility", "run service"}));
}

TEST(ApplicationTest, ReportsWhenTheServiceIsAlreadyRunning)
{
    FakePlatformBinding platform;
    platform.run_service_result.status = PlatformResultStatus::already_running;
    Application application(platform);

    EXPECT_EQ(application.Run(), 1);
    EXPECT_EQ(platform.run_service_count, 1);
    ExpectErrorReported(
        platform, {"Another instance of this application is already running in this interactive session."},
        {WindowVisibility::hidden, WindowVisibility::shown},
        {"set window visibility", "run service", "set window visibility", "write line", "wait for key"}, true);
}

TEST(ApplicationTest, ReportsAServiceFailure)
{
    FakePlatformBinding platform;
    platform.run_service_result = {PlatformResultStatus::failure, "service failed"};
    Application application(platform);

    EXPECT_EQ(application.Run(), 1);
    EXPECT_EQ(platform.run_service_count, 1);
    ExpectErrorReported(platform, {"service failed"}, {WindowVisibility::hidden, WindowVisibility::shown},
                        {"set window visibility", "run service", "set window visibility", "write line", "wait for key"},
                        true);
}

TEST(ApplicationTest, AppliesWindowCommandsReceivedByTheService)
{
    FakePlatformBinding platform;
    platform.received_window_commands = {WindowVisibility::shown, WindowVisibility::hidden};
    Application application(platform);

    EXPECT_EQ(application.Run(), 0);
    EXPECT_EQ(
        platform.window_visibility_changes,
        (std::vector<WindowVisibility>{WindowVisibility::hidden, WindowVisibility::shown, WindowVisibility::hidden}));
    EXPECT_EQ(platform.operations, (std::vector<std::string>{"set window visibility", "run service",
                                                             "set window visibility", "set window visibility"}));
}

TEST(ApplicationTest, SendsTheRequestedWindowCommand)
{
    for (const auto& [launch_command, expected_visibility] :
         {std::pair{LaunchCommand::show_window, WindowVisibility::shown},
          std::pair{LaunchCommand::hide_window, WindowVisibility::hidden}})
    {
        SCOPED_TRACE(static_cast<int>(launch_command));
        FakePlatformBinding platform;
        Application application(platform, launch_command);

        EXPECT_EQ(application.Run(), 0);
        EXPECT_EQ(platform.sent_window_commands, (std::vector<WindowVisibility>{expected_visibility}));
        EXPECT_EQ(platform.run_service_count, 0);
        EXPECT_EQ(platform.window_visibility_changes.size(), 0U);
        EXPECT_EQ(platform.written_lines.size(), 0U);
        EXPECT_EQ(platform.operations, (std::vector<std::string>{"send window command"}));
    }
}

TEST(ApplicationTest, ReportsWhenAWindowCommandHasNoRunningReceiver)
{
    FakePlatformBinding platform;
    platform.send_window_command_result.status = PlatformResultStatus::not_running;
    Application application(platform, LaunchCommand::show_window);

    EXPECT_EQ(application.Run(), 1);
    EXPECT_EQ(platform.sent_window_commands, (std::vector<WindowVisibility>{WindowVisibility::shown}));
    ExpectErrorReported(platform, {"No running instance in this interactive session is ready to receive commands."},
                        {WindowVisibility::shown}, {"send window command", "set window visibility", "write line"},
                        false);
}

TEST(ApplicationTest, ReportsAWindowCommandFailure)
{
    FakePlatformBinding platform;
    platform.send_window_command_result = {PlatformResultStatus::failure, "command failed"};
    Application application(platform, LaunchCommand::hide_window);

    EXPECT_EQ(application.Run(), 1);
    EXPECT_EQ(platform.sent_window_commands, (std::vector<WindowVisibility>{WindowVisibility::hidden}));
    ExpectErrorReported(platform, {"command failed"}, {WindowVisibility::shown},
                        {"send window command", "set window visibility", "write line"}, false);
}

TEST(ApplicationTest, ReservesTheInstanceAndSendsF13AfterFiveSeconds)
{
    FakePlatformBinding platform;
    Application application(platform, LaunchCommand::send_f13);

    EXPECT_EQ(application.Run(), 0);
    EXPECT_EQ(platform.reserve_single_instance_count, 1);
    EXPECT_EQ(platform.written_lines,
              (std::vector<std::string>{"F13 will be sent in 5 seconds. Focus the target application now."}));
    EXPECT_EQ(platform.waits, (std::vector<std::chrono::milliseconds>{std::chrono::seconds(5)}));
    EXPECT_EQ(platform.send_f13_count, 1);
    EXPECT_EQ(platform.operations, (std::vector<std::string>{"reserve instance", "write line", "wait", "send F13"}));
}

TEST(ApplicationTest, DoesNotSendF13WhileTheServiceIsRunning)
{
    FakePlatformBinding platform;
    platform.reserve_single_instance_result.status = PlatformResultStatus::already_running;
    Application application(platform, LaunchCommand::send_f13);

    EXPECT_EQ(application.Run(), 1);
    EXPECT_EQ(platform.reserve_single_instance_count, 1);
    EXPECT_EQ(platform.waits.size(), 0U);
    EXPECT_EQ(platform.send_f13_count, 0);
    ExpectErrorReported(platform,
                        {"Another instance of this application is already running in this interactive session. Close "
                         "it before sending F13."},
                        {WindowVisibility::shown}, {"reserve instance", "set window visibility", "write line"}, false);
}

TEST(ApplicationTest, ReportsAnInstanceReservationFailureWithoutSendingF13)
{
    FakePlatformBinding platform;
    platform.reserve_single_instance_result = {PlatformResultStatus::failure, "reservation failed"};
    Application application(platform, LaunchCommand::send_f13);

    EXPECT_EQ(application.Run(), 1);
    EXPECT_EQ(platform.reserve_single_instance_count, 1);
    EXPECT_EQ(platform.waits.size(), 0U);
    EXPECT_EQ(platform.send_f13_count, 0);
    ExpectErrorReported(platform, {"reservation failed"}, {WindowVisibility::shown},
                        {"reserve instance", "set window visibility", "write line"}, false);
}

TEST(ApplicationTest, ReportsAnF13InjectionFailure)
{
    FakePlatformBinding platform;
    platform.send_f13_result = {PlatformResultStatus::failure, "F13 failed"};
    Application application(platform, LaunchCommand::send_f13);

    EXPECT_EQ(application.Run(), 1);
    EXPECT_EQ(platform.reserve_single_instance_count, 1);
    EXPECT_EQ(platform.waits, (std::vector<std::chrono::milliseconds>{std::chrono::seconds(5)}));
    EXPECT_EQ(platform.send_f13_count, 1);
    ExpectErrorReported(platform, {"F13 will be sent in 5 seconds. Focus the target application now.", "F13 failed"},
                        {WindowVisibility::shown},
                        {"reserve instance", "write line", "wait", "send F13", "set window visibility", "write line"},
                        false);
}

TEST(ApplicationTest, ReportsUsageForAnInvalidLaunchCommand)
{
    FakePlatformBinding platform;
    Application application(platform, LaunchCommand::invalid);

    EXPECT_EQ(application.Run(), 1);
    EXPECT_EQ(platform.run_service_count, 0);
    EXPECT_EQ(platform.sent_window_commands.size(), 0U);
    EXPECT_EQ(platform.reserve_single_instance_count, 0);
    ExpectErrorReported(platform, {"Usage: DoubleClickHotkey [--start-shown | --show | --hide | --send-f13]"},
                        {WindowVisibility::shown}, {"set window visibility", "write line"}, false);
}

TEST(ApplicationTest, ReportsUsageForAnUnrecognizedLaunchCommand)
{
    FakePlatformBinding platform;
    Application application(platform, static_cast<LaunchCommand>(-1));

    EXPECT_EQ(application.Run(), 1);
    EXPECT_EQ(platform.run_service_count, 0);
    EXPECT_EQ(platform.sent_window_commands.size(), 0U);
    EXPECT_EQ(platform.reserve_single_instance_count, 0);
    ExpectErrorReported(platform, {"Usage: DoubleClickHotkey [--start-shown | --show | --hide | --send-f13]"},
                        {WindowVisibility::shown}, {"set window visibility", "write line"}, false);
}

TEST(ApplicationTest, DoubleClicksForAKeyPress)
{
    FakePlatformBinding platform;
    platform.hotkey_events.push_back({KeyTransition::pressed});
    Application application(platform);

    EXPECT_EQ(application.Run(), 0);

    EXPECT_EQ(platform.double_click_count, 1);
    EXPECT_EQ(platform.written_lines.size(), 0U);
    EXPECT_EQ(platform.operations, (std::vector<std::string>{"set window visibility", "run service", "double click"}));
}

TEST(ApplicationTest, DoubleClicksOnlyOncePerPhysicalPress)
{
    FakePlatformBinding platform;
    platform.hotkey_events = {
        {KeyTransition::pressed},  {KeyTransition::pressed}, {KeyTransition::pressed},
        {KeyTransition::released}, {KeyTransition::pressed}, {KeyTransition::released},
    };
    Application application(platform);

    EXPECT_EQ(application.Run(), 0);

    EXPECT_EQ(platform.double_click_count, 2);
    EXPECT_EQ(platform.operations,
              (std::vector<std::string>{"set window visibility", "run service", "double click", "double click"}));
}

TEST(ApplicationTest, DoesNotDoubleClickForAHotkeyRelease)
{
    FakePlatformBinding platform;
    platform.hotkey_events.push_back({KeyTransition::released});
    Application application(platform);

    EXPECT_EQ(application.Run(), 0);

    EXPECT_EQ(platform.double_click_count, 0);
    EXPECT_EQ(platform.operations, (std::vector<std::string>{"set window visibility", "run service"}));
}

TEST(ApplicationTest, LogsADoubleClickInjectionFailureWithoutShowingTheWindow)
{
    FakePlatformBinding platform;
    platform.hotkey_events.push_back({KeyTransition::pressed});
    platform.double_click_result = {PlatformResultStatus::failure, "double-click failed"};
    Application application(platform);

    EXPECT_EQ(application.Run(), 0);

    EXPECT_EQ(platform.double_click_count, 1);
    EXPECT_EQ(platform.written_lines, (std::vector<std::string>{"double-click failed"}));
    EXPECT_EQ(platform.window_visibility_changes, (std::vector<WindowVisibility>{WindowVisibility::hidden}));
    EXPECT_EQ(platform.wait_for_key_count, 0);
    EXPECT_EQ(platform.operations,
              (std::vector<std::string>{"set window visibility", "run service", "double click", "write line"}));
}
} // namespace
} // namespace double_click_hotkey
