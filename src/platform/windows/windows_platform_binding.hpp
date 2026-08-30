#pragma once

#include "double_click_hotkey/platform_binding.hpp"
#include "platform/windows/console.hpp"
#include "platform/windows/instance_command.hpp"
#include "platform/windows/keyboard_sender.hpp"
#include "platform/windows/mouse.hpp"

#include <string>

namespace double_click_hotkey::windows
{
class WindowsPlatformBinding final : public PlatformBinding
{
  public:
    explicit WindowsPlatformBinding(bool hide_window_immediately) noexcept;

    [[nodiscard]] int Run(LaunchCommand launch_command, HotkeyEventHandler handler) override;
    void DoubleClick() override;

  private:
    [[nodiscard]] int RunApplication(HotkeyEventHandler handler);
    [[nodiscard]] int SendConsoleCommand(InstanceCommand command);
    [[nodiscard]] int SendF13AfterDelay();
    [[nodiscard]] static std::string FormatError(const char* message, unsigned long error_code);
    [[nodiscard]] std::string RunMessageLoop(HotkeyEventHandler handler,
                                             const InstanceCommandReceiver& command_receiver);

    Console console_;
    KeyboardSender keyboard_sender_;
    Mouse mouse_;
};
} // namespace double_click_hotkey::windows
