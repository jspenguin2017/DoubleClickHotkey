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
    int Run(HotkeyEventHandler handler) override
    {
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

    void ToggleConsoleVisibility() override
    {
        ++console_toggle_count;
    }

    std::vector<HotkeyEvent> events;
    std::vector<bool> handled_events;
    int exit_code = 0;
    int double_click_count = 0;
    int console_toggle_count = 0;
};

TEST(ApplicationTest, ReturnsThePlatformExitCode)
{
    FakePlatformBinding platform;
    platform.exit_code = 23;
    Application application(platform);

    EXPECT_EQ(application.Run(), 23);
}

TEST(ApplicationTest, DoubleClicksAndConsumesAnUnmodifiedKeyPress)
{
    FakePlatformBinding platform;
    platform.events.push_back({KeyTransition::pressed, {}});
    Application application(platform);

    static_cast<void>(application.Run());

    EXPECT_EQ(platform.double_click_count, 1);
    EXPECT_EQ(platform.console_toggle_count, 0);
    ASSERT_EQ(platform.handled_events.size(), 1U);
    EXPECT_TRUE(platform.handled_events.front());
}

TEST(ApplicationTest, ConsumesAHotkeyReleaseWithoutRepeatingItsAction)
{
    FakePlatformBinding platform;
    platform.events.push_back({KeyTransition::released, {}});
    Application application(platform);

    static_cast<void>(application.Run());

    EXPECT_EQ(platform.double_click_count, 0);
    ASSERT_EQ(platform.handled_events.size(), 1U);
    EXPECT_TRUE(platform.handled_events.front());
}

TEST(ApplicationTest, TogglesTheConsoleForTheModifierCombination)
{
    ModifierState modifiers;
    modifiers.alt = true;
    modifiers.control = true;
    modifiers.shift = true;

    FakePlatformBinding platform;
    platform.events.push_back({KeyTransition::pressed, modifiers});
    Application application(platform);

    static_cast<void>(application.Run());

    EXPECT_EQ(platform.double_click_count, 0);
    EXPECT_EQ(platform.console_toggle_count, 1);
    ASSERT_EQ(platform.handled_events.size(), 1U);
    EXPECT_TRUE(platform.handled_events.front());
}

TEST(ApplicationTest, ForwardsAnUnsupportedModifierCombination)
{
    ModifierState modifiers;
    modifiers.control = true;

    FakePlatformBinding platform;
    platform.events.push_back({KeyTransition::pressed, modifiers});
    Application application(platform);

    static_cast<void>(application.Run());

    EXPECT_EQ(platform.double_click_count, 0);
    EXPECT_EQ(platform.console_toggle_count, 0);
    ASSERT_EQ(platform.handled_events.size(), 1U);
    EXPECT_FALSE(platform.handled_events.front());
}
} // namespace
} // namespace double_click_hotkey
