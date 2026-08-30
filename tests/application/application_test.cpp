#include "double_click_hotkey/application.hpp"

#include <gtest/gtest.h>

#include <vector>

namespace double_click_hotkey
{
namespace
{
class FakePlatformBinding final : public PlatformBinding
{
  public:
    int Run(const LaunchCommand launch_command, HotkeyEventHandler handler) override
    {
        received_launch_command = launch_command;
        for (const HotkeyEvent& event : events)
        {
            handled_events.push_back(handler(event));
        }
        return exit_code;
    }

    void DoubleClick() override
    {
        ++double_click_count;
    }

    std::vector<HotkeyEvent> events;
    std::vector<bool> handled_events;
    LaunchCommand received_launch_command = LaunchCommand::invalid;
    int exit_code = 0;
    int double_click_count = 0;
};

TEST(ApplicationTest, ReturnsThePlatformExitCode)
{
    FakePlatformBinding platform;
    platform.exit_code = 23;
    Application application(platform);

    EXPECT_EQ(application.Run(), 23);
    EXPECT_EQ(platform.received_launch_command, LaunchCommand::run);
}

TEST(ApplicationTest, PassesTheLaunchCommandToThePlatform)
{
    FakePlatformBinding platform;
    Application application(platform, LaunchCommand::show_window);

    static_cast<void>(application.Run());

    EXPECT_EQ(platform.received_launch_command, LaunchCommand::show_window);
}

TEST(ApplicationTest, DoubleClicksAndConsumesAnUnmodifiedKeyPress)
{
    FakePlatformBinding platform;
    platform.events.push_back({KeyTransition::pressed, {}});
    Application application(platform);

    static_cast<void>(application.Run());

    EXPECT_EQ(platform.double_click_count, 1);
    ASSERT_EQ(platform.handled_events.size(), 1U);
    EXPECT_TRUE(platform.handled_events.front());
}

TEST(ApplicationTest, ConsumesAModifiedHotkeyReleaseWithoutRepeatingItsAction)
{
    ModifierState modifiers;
    modifiers.alt = true;
    modifiers.control = true;
    modifiers.shift = true;
    modifiers.system = true;

    FakePlatformBinding platform;
    platform.events.push_back({KeyTransition::released, modifiers});
    Application application(platform);

    static_cast<void>(application.Run());

    EXPECT_EQ(platform.double_click_count, 0);
    ASSERT_EQ(platform.handled_events.size(), 1U);
    EXPECT_TRUE(platform.handled_events.front());
}

TEST(ApplicationTest, DoubleClicksAndConsumesAPressWithEveryModifier)
{
    ModifierState modifiers;
    modifiers.alt = true;
    modifiers.control = true;
    modifiers.shift = true;

    FakePlatformBinding platform;
    platform.events.push_back({KeyTransition::pressed, modifiers});
    Application application(platform);

    static_cast<void>(application.Run());

    EXPECT_EQ(platform.double_click_count, 1);
    ASSERT_EQ(platform.handled_events.size(), 1U);
    EXPECT_TRUE(platform.handled_events.front());
}

TEST(ApplicationTest, DoubleClicksAndConsumesAPressWithASingleModifier)
{
    ModifierState modifiers;
    modifiers.control = true;

    FakePlatformBinding platform;
    platform.events.push_back({KeyTransition::pressed, modifiers});
    Application application(platform);

    static_cast<void>(application.Run());

    EXPECT_EQ(platform.double_click_count, 1);
    ASSERT_EQ(platform.handled_events.size(), 1U);
    EXPECT_TRUE(platform.handled_events.front());
}
} // namespace
} // namespace double_click_hotkey
