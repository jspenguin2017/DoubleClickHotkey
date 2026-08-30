#pragma once

#include "platform/windows/input_injector.hpp"

#include <windows.h>

#include <optional>

namespace double_click_hotkey::windows
{
class KeyboardSender
{
  public:
    using SendInputFunction = InputInjector::SendInputFunction;

    explicit KeyboardSender(SendInputFunction send_input = &::SendInput) noexcept;

    [[nodiscard]] bool SendF13() noexcept;
    [[nodiscard]] DWORD LastErrorCode() const noexcept;
    [[nodiscard]] std::optional<DWORD> LastReleaseErrorCode() const noexcept;

  private:
    InputInjector input_injector_;
};
} // namespace double_click_hotkey::windows
