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

    handler_ = std::move(handler);
    active_hook_ = this;
    handle_ = SetWindowsHookExW(WH_KEYBOARD_LL, &HandleKeyboardEvent, nullptr, 0);
    if (handle_ == nullptr)
    {
        last_error_code_ = GetLastError();
        active_hook_ = nullptr;
        handler_ = {};
        return false;
    }

    last_error_code_ = ERROR_SUCCESS;
    return true;
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

ModifierState KeyboardHook::GetModifierState() noexcept
{
    return {
        GetKeyState(VK_MENU) < 0,
        GetKeyState(VK_CONTROL) < 0,
        GetKeyState(VK_SHIFT) < 0,
        GetKeyState(VK_LWIN) < 0 || GetKeyState(VK_RWIN) < 0,
    };
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

    const HotkeyEvent event{
        is_key_down ? KeyTransition::pressed : KeyTransition::released,
        GetModifierState(),
    };
    if (handler_(event))
    {
        return 1;
    }

    return CallNextHookEx(handle_, code, message, data);
}
} // namespace double_click_hotkey::windows
