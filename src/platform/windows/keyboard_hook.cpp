#include "platform/windows/keyboard_hook.hpp"

#include <utility>

namespace double_click_hotkey::windows
{
KeyboardHook* KeyboardHook::active_hook_ = nullptr;

KeyboardHook::~KeyboardHook()
{
    if (active_hook_ == this)
    {
        active_hook_ = nullptr;
    }

    if (handle_ != nullptr)
    {
        static_cast<void>(UnhookWindowsHookEx(handle_));
    }
}

bool KeyboardHook::Install(HotkeyEventHandler handler)
{
    if (handle_ != nullptr || active_hook_ != nullptr)
    {
        last_error_code_ = ERROR_ALREADY_EXISTS;
        return false;
    }

    MSG message{};
    static_cast<void>(PeekMessageW(&message, nullptr, WM_USER, WM_USER, PM_NOREMOVE));

    handler_ = std::move(handler);
    owner_thread_id_ = GetCurrentThreadId();
    active_hook_ = this;
    handle_ = SetWindowsHookExW(WH_KEYBOARD_LL, &HandleKeyboardEvent, nullptr, 0);
    if (handle_ == nullptr)
    {
        last_error_code_ = GetLastError();
        active_hook_ = nullptr;
        handler_ = {};
        owner_thread_id_ = 0;
        return false;
    }

    last_error_code_ = ERROR_SUCCESS;
    hotkey_is_pressed_ = false;
    event_queue_failed_ = false;
    return true;
}

bool KeyboardHook::HandleQueuedEvent(const MSG& message)
{
    if (message.hwnd != nullptr || message.message != HotkeyEventMessage ||
        (message.wParam != static_cast<WPARAM>(KeyTransition::pressed) &&
         message.wParam != static_cast<WPARAM>(KeyTransition::released)))
    {
        return false;
    }

    handler_({static_cast<KeyTransition>(message.wParam)});
    return true;
}

bool KeyboardHook::EventQueueFailed() const noexcept
{
    return event_queue_failed_;
}

DWORD KeyboardHook::LastErrorCode() const noexcept
{
    return last_error_code_;
}

LRESULT CALLBACK KeyboardHook::HandleKeyboardEvent(const int code, const WPARAM message, const LPARAM data)
{
    if (active_hook_ == nullptr)
    {
        return CallNextHookEx(nullptr, code, message, data);
    }

    return active_hook_->DispatchKeyboardEvent(code, message, data);
}

LRESULT KeyboardHook::DispatchKeyboardEvent(const int code, const WPARAM message, const LPARAM data)
{
    if (code < 0)
    {
        return CallNextHookEx(handle_, code, message, data);
    }

    const bool is_key_down = message == WM_KEYDOWN || message == WM_SYSKEYDOWN;
    const bool is_key_up = message == WM_KEYUP || message == WM_SYSKEYUP;
    if (!is_key_down && !is_key_up)
    {
        return CallNextHookEx(handle_, code, message, data);
    }

    const auto* keyboard_event = reinterpret_cast<PKBDLLHOOKSTRUCT>(data);
    if (keyboard_event->vkCode != VK_F13)
    {
        return CallNextHookEx(handle_, code, message, data);
    }

    if (is_key_down)
    {
        if (!hotkey_is_pressed_)
        {
            hotkey_is_pressed_ = true;
            QueueEvent(KeyTransition::pressed);
        }
        return 1;
    }

    if (hotkey_is_pressed_)
    {
        hotkey_is_pressed_ = false;
        QueueEvent(KeyTransition::released);
    }
    return 1;
}

void KeyboardHook::QueueEvent(const KeyTransition transition) noexcept
{
    if (PostThreadMessageW(owner_thread_id_, HotkeyEventMessage, static_cast<WPARAM>(transition), 0) == 0)
    {
        last_error_code_ = GetLastError();
        event_queue_failed_ = true;
    }
}
} // namespace double_click_hotkey::windows
