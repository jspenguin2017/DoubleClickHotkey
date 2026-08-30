#pragma once

#include <string_view>
#include <vector>

namespace double_click_hotkey
{
enum class LaunchCommand
{
    run,
    start_shown,
    show_window,
    hide_window,
    send_f13,
    invalid,
};

[[nodiscard]] LaunchCommand ParseLaunchCommand(const std::vector<std::string_view>& arguments) noexcept;
} // namespace double_click_hotkey
