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
    [[nodiscard]] bool HandleQueuedEvent(const MSG& message);
    [[nodiscard]] bool EventQueueFailed() const noexcept;
    [[nodiscard]] DWORD LastErrorCode() const noexcept;

  private:
    static LRESULT CALLBACK HandleKeyboardEvent(int code, WPARAM message, LPARAM data);
    [[nodiscard]] LRESULT DispatchKeyboardEvent(int code, WPARAM message, LPARAM data);
    void QueueEvent(KeyTransition transition) noexcept;

    static KeyboardHook* active_hook_;
    static constexpr UINT HotkeyEventMessage = WM_APP;
    HHOOK handle_ = nullptr;
    HotkeyEventHandler handler_;
    DWORD owner_thread_id_ = 0;
    DWORD last_error_code_ = ERROR_SUCCESS;
    bool hotkey_is_pressed_ = false;
    bool event_queue_failed_ = false;
};
} // namespace double_click_hotkey::windows
