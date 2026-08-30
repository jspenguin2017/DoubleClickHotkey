#pragma once

#include "double_click_hotkey/platform_binding.hpp"

#include <windows.h>

namespace double_click_hotkey::windows
{
class KeyboardHook
{
  public:
    KeyboardHook() = default;
    ~KeyboardHook();

    KeyboardHook(const KeyboardHook&) = delete;
    KeyboardHook& operator=(const KeyboardHook&) = delete;
    KeyboardHook(KeyboardHook&&) = delete;
    KeyboardHook& operator=(KeyboardHook&&) = delete;

    [[nodiscard]] bool Install(HotkeyEventHandler handler);
    [[nodiscard]] DWORD LastErrorCode() const noexcept;

  private:
    static LRESULT CALLBACK HandleKeyboardEvent(int code, WPARAM message, LPARAM data);
    [[nodiscard]] LRESULT DispatchKeyboardEvent(int code, WPARAM message, LPARAM data);

    static KeyboardHook* active_hook_;
    HHOOK handle_ = nullptr;
    HotkeyEventHandler handler_;
    DWORD last_error_code_ = ERROR_SUCCESS;
};
} // namespace double_click_hotkey::windows
