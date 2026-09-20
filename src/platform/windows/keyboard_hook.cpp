#include "platform/windows/keyboard_hook.hpp"

#include "platform/windows/input_tag.hpp"

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

bool KeyboardHook::Install(HWND receiver)
{
    if (handle_ != nullptr || active_hook_ != nullptr)
    {
        last_error_code_ = ERROR_ALREADY_EXISTS;
        return false;
    }

    const HMODULE executable_module = GetModuleHandleW(nullptr);
    if (executable_module == nullptr)
    {
        last_error_code_ = GetLastError();
        return false;
    }

    MSG message{};
    static_cast<void>(PeekMessageW(&message, nullptr, WM_USER, WM_USER, PM_NOREMOVE));

    receiver_ = receiver;
    active_hook_ = this;
    handle_ = SetWindowsHookExW(WH_KEYBOARD_LL, &HandleKeyboardEvent, executable_module, 0);
    if (handle_ == nullptr)
    {
        last_error_code_ = GetLastError();
        active_hook_ = nullptr;
        receiver_ = nullptr;
        return false;
    }

    last_error_code_ = ERROR_SUCCESS;
    hotkey_is_pressed_ = false;
    pass_hotkey_through_until_release_ = GetAsyncKeyState(VK_F13) < 0;
    event_queue_failed_ = false;
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

    // Our setup keystroke must reach the foreground app without changing physical-key tracking.
    if ((keyboard_event->flags & LLKHF_INJECTED) != 0 && keyboard_event->dwExtraInfo == F13InputTag())
    {
        return CallNextHookEx(handle_, code, message, data);
    }

    if (pass_hotkey_through_until_release_)
    {
        if (is_key_up)
        {
            pass_hotkey_through_until_release_ = false;
        }
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
        return 1;
    }

    return CallNextHookEx(handle_, code, message, data);
}

void KeyboardHook::QueueEvent(const KeyTransition transition) noexcept
{
    if (PostMessageW(receiver_, HotkeyEventMessage, static_cast<WPARAM>(transition), 0) == 0)
    {
        last_error_code_ = GetLastError();
        event_queue_failed_ = true;
    }
}
} // namespace double_click_hotkey::windows
