#pragma once

#include "double_click_hotkey/hotkey_policy.hpp"

#include <windows.h>

namespace double_click_hotkey::windows
{
class InputThreads;

// Construct, install, and destroy on the hook thread only.
class KeyboardHook
{
  public:
    KeyboardHook() = default;
    ~KeyboardHook();

    KeyboardHook(const KeyboardHook&) = delete;
    KeyboardHook& operator=(const KeyboardHook&) = delete;
    KeyboardHook(KeyboardHook&&) = delete;
    KeyboardHook& operator=(KeyboardHook&&) = delete;

    [[nodiscard]] bool Install(InputThreads& receiver);
    [[nodiscard]] DWORD LastErrorCode() const noexcept;

  private:
    static LRESULT CALLBACK HandleKeyboardEvent(int code, WPARAM message, LPARAM data) noexcept;
    [[nodiscard]] LRESULT DispatchKeyboardEvent(int code, WPARAM message, LPARAM data) noexcept;

    static KeyboardHook* active_hook_;
    HHOOK handle_ = nullptr;
    InputThreads* receiver_ = nullptr;
    DWORD last_error_code_ = ERROR_SUCCESS;
    HotkeyPolicy policy_;
};
} // namespace double_click_hotkey::windows
