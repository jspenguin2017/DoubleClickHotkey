#include "double_click_hotkey/launch_command.hpp"

#include <gtest/gtest.h>

#include <string_view>
#include <vector>

namespace double_click_hotkey
{
TEST(LaunchCommandTest, RunsNormallyWithoutArguments)
{
    EXPECT_EQ(ParseLaunchCommand({}), LaunchCommand::run);
}

TEST(LaunchCommandTest, ParsesShow)
{
    EXPECT_EQ(ParseLaunchCommand({"--show"}), LaunchCommand::show_window);
}

TEST(LaunchCommandTest, ParsesHide)
{
    EXPECT_EQ(ParseLaunchCommand({"--hide"}), LaunchCommand::hide_window);
}

TEST(LaunchCommandTest, ParsesSendF13)
{
    EXPECT_EQ(ParseLaunchCommand({"--send-f13"}), LaunchCommand::send_f13);
}

TEST(LaunchCommandTest, RejectsAnUnknownArgument)
{
    EXPECT_EQ(ParseLaunchCommand({"--unknown"}), LaunchCommand::invalid);
}

TEST(LaunchCommandTest, RejectsMultipleArguments)
{
    EXPECT_EQ(ParseLaunchCommand({"--show", "--hide"}), LaunchCommand::invalid);
}
} // namespace double_click_hotkey
