#pragma once

#include "double_click_hotkey/launch_command.hpp"
#include "double_click_hotkey/platform_binding.hpp"

namespace double_click_hotkey
{
class Application
{
  public:
    explicit Application(PlatformBinding& platform, LaunchCommand launch_command = LaunchCommand::run) noexcept;

    [[nodiscard]] int Run();

  private:
    [[nodiscard]] bool HandleHotkeyEvent(const HotkeyEvent& event);

    PlatformBinding& platform_;
    LaunchCommand launch_command_;
};
} // namespace double_click_hotkey
