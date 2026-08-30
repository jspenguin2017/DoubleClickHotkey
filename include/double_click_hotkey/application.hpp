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
    [[nodiscard]] int RunService();
    [[nodiscard]] int SendWindowCommand(WindowVisibility visibility);
    [[nodiscard]] int SendF13AfterDelay();
    [[nodiscard]] bool HandleHotkeyEvent(const HotkeyEvent& event);
    void HandleWindowVisibility(WindowVisibility visibility);
    void ReportError(std::string_view message, bool wait_for_key);
    void ReportResultError(const PlatformResult& result, bool wait_for_key = false);

    PlatformBinding& platform_;
    LaunchCommand launch_command_;
    bool hotkey_is_pressed_ = false;
};
} // namespace double_click_hotkey
