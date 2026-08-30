#pragma once

#include "double_click_hotkey/platform_binding.hpp"
#include "platform/windows/console.hpp"
#include "platform/windows/instance_command.hpp"
#include "platform/windows/keyboard_sender.hpp"
#include "platform/windows/mouse.hpp"
#include "platform/windows/single_instance.hpp"

#include <chrono>
#include <memory>
#include <string>
#include <string_view>

namespace double_click_hotkey::windows
{
class WindowsPlatformBinding final : public PlatformBinding
{
  public:
    [[nodiscard]] PlatformResult RunService(HotkeyEventHandler hotkey_handler,
                                            WindowVisibilityHandler visibility_handler) override;

    [[nodiscard]] PlatformResult ReserveSingleInstance() override;
    [[nodiscard]] PlatformResult SendWindowCommand(WindowVisibility visibility) override;

    void SetWindowVisibility(WindowVisibility visibility) override;
    void WriteLine(std::string_view message) override;
    void WaitFor(std::chrono::milliseconds duration) override;
    void WaitForKey() override;

    [[nodiscard]] PlatformResult SendF13() override;
    [[nodiscard]] PlatformResult DoubleClick() override;

  private:
    [[nodiscard]] static std::string FormatError(const char* message, unsigned long error_code);
    [[nodiscard]] static InstanceCommand ToInstanceCommand(WindowVisibility visibility) noexcept;
    [[nodiscard]] PlatformResult RunMessageLoop(HotkeyEventHandler hotkey_handler,
                                                WindowVisibilityHandler visibility_handler,
                                                const InstanceCommandReceiver& command_receiver);

    Console console_;
    KeyboardSender keyboard_sender_;
    Mouse mouse_;
    std::unique_ptr<SingleInstance> single_instance_reservation_;
};
} // namespace double_click_hotkey::windows
