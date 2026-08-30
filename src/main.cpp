#include "double_click_hotkey/application.hpp"
#include "double_click_hotkey/launch_command.hpp"
#include "double_click_hotkey/platform_factory.hpp"

#include <string_view>
#include <vector>

int main(const int argument_count, char* arguments[])
{
    const bool has_launch_arguments = argument_count > 1;
    const auto platform = double_click_hotkey::CreatePlatformBinding(!has_launch_arguments);

    std::vector<std::string_view> launch_arguments;
    if (has_launch_arguments)
    {
        launch_arguments.reserve(static_cast<std::size_t>(argument_count - 1));
    }
    for (int index = 1; index < argument_count; ++index)
    {
        launch_arguments.emplace_back(arguments[index]);
    }

    const double_click_hotkey::LaunchCommand launch_command = double_click_hotkey::ParseLaunchCommand(launch_arguments);
    double_click_hotkey::Application application(*platform, launch_command);
    return application.Run();
}
