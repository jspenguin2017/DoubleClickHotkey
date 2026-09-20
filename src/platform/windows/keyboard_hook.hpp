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

    static constexpr UINT HotkeyEventMessage = WM_APP + 2;
    [[nodiscard]] bool Install(HWND receiver);
    [[nodiscard]] bool EventQueueFailed() const noexcept;
    [[nodiscard]] DWORD LastErrorCode() const noexcept;

  private:
    static LRESULT CALLBACK HandleKeyboardEvent(int code, WPARAM message, LPARAM data);
    [[nodiscard]] LRESULT DispatchKeyboardEvent(int code, WPARAM message, LPARAM data);
    void QueueEvent(KeyTransition transition) noexcept;

    static KeyboardHook* active_hook_;
    HHOOK handle_ = nullptr;
    HWND receiver_ = nullptr;
    DWORD last_error_code_ = ERROR_SUCCESS;
    bool hotkey_is_pressed_ = false;
    bool pass_hotkey_through_until_release_ = false;
    bool event_queue_failed_ = false;
};
} // namespace double_click_hotkey::windows
