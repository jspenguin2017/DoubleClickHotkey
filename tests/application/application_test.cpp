#include "double_click_hotkey/application.hpp"

#include <gtest/gtest.h>

#include <chrono>
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
        ++run_service_count;
        for (const HotkeyEvent& event : hotkey_events)
        {
            handled_hotkey_events.push_back(hotkey_handler(event));
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
        sent_window_commands.push_back(visibility);
        return send_window_command_result;
    }

    void SetWindowVisibility(const WindowVisibility visibility) override
    {
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
        ++double_click_count;
        return double_click_result;
    }

    PlatformResult run_service_result;
    PlatformResult reserve_single_instance_result;
    PlatformResult send_window_command_result;
    PlatformResult send_f13_result;
    PlatformResult double_click_result;
    std::vector<HotkeyEvent> hotkey_events;
    std::vector<bool> handled_hotkey_events;
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

void ExpectErrorReported(const FakePlatformBinding& platform, const std::string_view message, const bool waited_for_key)
{
    ASSERT_FALSE(platform.written_lines.empty());
    EXPECT_EQ(platform.written_lines.back(), message);
    ASSERT_FALSE(platform.window_visibility_changes.empty());
    EXPECT_EQ(platform.window_visibility_changes.back(), WindowVisibility::shown);
    EXPECT_EQ(platform.wait_for_key_count, waited_for_key ? 1 : 0);
}

TEST(ApplicationTest, RunsTheServiceAndHidesItsWindow)
{
    FakePlatformBinding platform;
    Application application(platform);

    EXPECT_EQ(application.Run(), 0);
    EXPECT_EQ(platform.run_service_count, 1);
    ASSERT_EQ(platform.window_visibility_changes.size(), 1U);
    EXPECT_EQ(platform.window_visibility_changes.front(), WindowVisibility::hidden);
}

TEST(ApplicationTest, ReportsWhenTheServiceIsAlreadyRunning)
{
    FakePlatformBinding platform;
    platform.run_service_result.status = PlatformResultStatus::already_running;
    Application application(platform);

    EXPECT_EQ(application.Run(), 1);
    ExpectErrorReported(platform, "Another instance of this application is already running.", true);
}

TEST(ApplicationTest, ReportsAServiceFailure)
{
    FakePlatformBinding platform;
    platform.run_service_result = {PlatformResultStatus::failure, "service failed"};
    Application application(platform);

    EXPECT_EQ(application.Run(), 1);
    ExpectErrorReported(platform, "service failed", true);
}

TEST(ApplicationTest, AppliesWindowCommandsReceivedByTheService)
{
    FakePlatformBinding platform;
    platform.received_window_commands = {WindowVisibility::shown, WindowVisibility::hidden};
    Application application(platform);

    EXPECT_EQ(application.Run(), 0);
    ASSERT_EQ(platform.window_visibility_changes.size(), 3U);
    EXPECT_EQ(platform.window_visibility_changes[0], WindowVisibility::hidden);
    EXPECT_EQ(platform.window_visibility_changes[1], WindowVisibility::shown);
    EXPECT_EQ(platform.window_visibility_changes[2], WindowVisibility::hidden);
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
        ASSERT_EQ(platform.sent_window_commands.size(), 1U);
        EXPECT_EQ(platform.sent_window_commands.front(), expected_visibility);
        EXPECT_EQ(platform.run_service_count, 0);
    }
}

TEST(ApplicationTest, ReportsWhenAWindowCommandHasNoRunningReceiver)
{
    FakePlatformBinding platform;
    platform.send_window_command_result.status = PlatformResultStatus::not_running;
    Application application(platform, LaunchCommand::show_window);

    EXPECT_EQ(application.Run(), 1);
    ExpectErrorReported(platform, "No running instance of this application was found.", false);
}

TEST(ApplicationTest, ReportsAWindowCommandFailure)
{
    FakePlatformBinding platform;
    platform.send_window_command_result = {PlatformResultStatus::failure, "command failed"};
    Application application(platform, LaunchCommand::hide_window);

    EXPECT_EQ(application.Run(), 1);
    ExpectErrorReported(platform, "command failed", false);
}

TEST(ApplicationTest, ReservesTheInstanceAndSendsF13AfterFiveSeconds)
{
    FakePlatformBinding platform;
    Application application(platform, LaunchCommand::send_f13);

    EXPECT_EQ(application.Run(), 0);
    EXPECT_EQ(platform.reserve_single_instance_count, 1);
    ASSERT_EQ(platform.written_lines.size(), 1U);
    EXPECT_EQ(platform.written_lines.front(), "F13 will be sent in 5 seconds. Focus the target application now.");
    ASSERT_EQ(platform.waits.size(), 1U);
    EXPECT_EQ(platform.waits.front(), std::chrono::seconds(5));
    EXPECT_EQ(platform.send_f13_count, 1);
    EXPECT_EQ(platform.operations, (std::vector<std::string>{"reserve instance", "write line", "wait", "send F13"}));
}

TEST(ApplicationTest, DoesNotSendF13WhileTheServiceIsRunning)
{
    FakePlatformBinding platform;
    platform.reserve_single_instance_result.status = PlatformResultStatus::already_running;
    Application application(platform, LaunchCommand::send_f13);

    EXPECT_EQ(application.Run(), 1);
    EXPECT_TRUE(platform.waits.empty());
    EXPECT_EQ(platform.send_f13_count, 0);
    ExpectErrorReported(platform,
                        "Another instance of this application is already running. Close it before sending F13.", false);
}

TEST(ApplicationTest, ReportsAnInstanceReservationFailureWithoutSendingF13)
{
    FakePlatformBinding platform;
    platform.reserve_single_instance_result = {PlatformResultStatus::failure, "reservation failed"};
    Application application(platform, LaunchCommand::send_f13);

    EXPECT_EQ(application.Run(), 1);
    EXPECT_TRUE(platform.waits.empty());
    EXPECT_EQ(platform.send_f13_count, 0);
    ExpectErrorReported(platform, "reservation failed", false);
}

TEST(ApplicationTest, ReportsAnF13InjectionFailure)
{
    FakePlatformBinding platform;
    platform.send_f13_result = {PlatformResultStatus::failure, "F13 failed"};
    Application application(platform, LaunchCommand::send_f13);

    EXPECT_EQ(application.Run(), 1);
    EXPECT_EQ(platform.send_f13_count, 1);
    ExpectErrorReported(platform, "F13 failed", false);
}

TEST(ApplicationTest, ReportsUsageForAnInvalidLaunchCommand)
{
    FakePlatformBinding platform;
    Application application(platform, LaunchCommand::invalid);

    EXPECT_EQ(application.Run(), 1);
    EXPECT_EQ(platform.run_service_count, 0);
    ExpectErrorReported(platform, "Usage: DoubleClickHotkey [--show | --hide | --send-f13]", false);
}

TEST(ApplicationTest, DoubleClicksAndConsumesAnUnmodifiedKeyPress)
{
    FakePlatformBinding platform;
    platform.hotkey_events.push_back({KeyTransition::pressed, {}});
    Application application(platform);

    static_cast<void>(application.Run());

    EXPECT_EQ(platform.double_click_count, 1);
    ASSERT_EQ(platform.handled_hotkey_events.size(), 1U);
    EXPECT_TRUE(platform.handled_hotkey_events.front());
}

TEST(ApplicationTest, DoubleClicksOnlyOncePerPhysicalPress)
{
    FakePlatformBinding platform;
    platform.hotkey_events = {
        {KeyTransition::pressed, {}},  {KeyTransition::pressed, {}}, {KeyTransition::pressed, {}},
        {KeyTransition::released, {}}, {KeyTransition::pressed, {}}, {KeyTransition::released, {}},
    };
    Application application(platform);

    static_cast<void>(application.Run());

    EXPECT_EQ(platform.double_click_count, 2);
    EXPECT_EQ(platform.handled_hotkey_events, (std::vector<bool>{true, true, true, true, true, true}));
}

TEST(ApplicationTest, ConsumesAModifiedHotkeyReleaseWithoutRepeatingItsAction)
{
    ModifierState modifiers;
    modifiers.alt = true;
    modifiers.control = true;
    modifiers.shift = true;
    modifiers.system = true;

    FakePlatformBinding platform;
    platform.hotkey_events.push_back({KeyTransition::released, modifiers});
    Application application(platform);

    static_cast<void>(application.Run());

    EXPECT_EQ(platform.double_click_count, 0);
    ASSERT_EQ(platform.handled_hotkey_events.size(), 1U);
    EXPECT_TRUE(platform.handled_hotkey_events.front());
}

TEST(ApplicationTest, DoubleClicksAndConsumesAPressWithEveryModifier)
{
    ModifierState modifiers;
    modifiers.alt = true;
    modifiers.control = true;
    modifiers.shift = true;
    modifiers.system = true;

    FakePlatformBinding platform;
    platform.hotkey_events.push_back({KeyTransition::pressed, modifiers});
    Application application(platform);

    static_cast<void>(application.Run());

    EXPECT_EQ(platform.double_click_count, 1);
    ASSERT_EQ(platform.handled_hotkey_events.size(), 1U);
    EXPECT_TRUE(platform.handled_hotkey_events.front());
}

TEST(ApplicationTest, ReportsADoubleClickInjectionFailureAndStillConsumesTheHotkey)
{
    FakePlatformBinding platform;
    platform.hotkey_events.push_back({KeyTransition::pressed, {}});
    platform.double_click_result = {PlatformResultStatus::failure, "double-click failed"};
    Application application(platform);

    static_cast<void>(application.Run());

    ExpectErrorReported(platform, "double-click failed", false);
    ASSERT_EQ(platform.handled_hotkey_events.size(), 1U);
    EXPECT_TRUE(platform.handled_hotkey_events.front());
}
} // namespace
} // namespace double_click_hotkey
