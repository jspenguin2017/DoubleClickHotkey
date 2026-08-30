#include "double_click_hotkey/launch_command.hpp"

namespace double_click_hotkey
{
LaunchCommand ParseLaunchCommand(const std::vector<std::string_view>& arguments) noexcept
{
    if (arguments.empty())
    {
        return LaunchCommand::run;
    }
    if (arguments.size() != 1)
    {
        return LaunchCommand::invalid;
    }

    const std::string_view argument = arguments.front();
    if (argument == "--start-shown")
    {
        return LaunchCommand::start_shown;
    }
    if (argument == "--show")
    {
        return LaunchCommand::show_window;
    }
    if (argument == "--hide")
    {
        return LaunchCommand::hide_window;
    }
    if (argument == "--send-f13")
    {
        return LaunchCommand::send_f13;
    }

    return LaunchCommand::invalid;
}
} // namespace double_click_hotkey
