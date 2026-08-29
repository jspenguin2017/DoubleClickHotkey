#pragma once

#include "double_click_hotkey/platform_binding.hpp"

namespace double_click_hotkey
{
class Application
{
  public:
    explicit Application(PlatformBinding& platform) noexcept;

    [[nodiscard]] int Run();

  private:
    [[nodiscard]] bool HandleHotkeyEvent(const HotkeyEvent& event);

    PlatformBinding& platform_;
};
} // namespace double_click_hotkey
