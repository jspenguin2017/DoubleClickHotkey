#pragma once

#include "double_click_hotkey/platform_binding.hpp"
#include "platform/windows/console.hpp"
#include "platform/windows/mouse.hpp"

#include <string>

namespace double_click_hotkey::windows
{
class WindowsPlatformBinding final : public PlatformBinding
{
  public:
    [[nodiscard]] int Run(HotkeyEventHandler handler) override;
    void DoubleClick() override;
    void ToggleConsoleVisibility() override;

  private:
    [[nodiscard]] static std::string FormatError(const char* message, unsigned long error_code);
    [[nodiscard]] static std::string RunMessageLoop(HotkeyEventHandler handler);

    Console console_;
    Mouse mouse_;
};
} // namespace double_click_hotkey::windows
