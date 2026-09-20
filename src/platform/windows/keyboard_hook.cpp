#include "platform/windows/keyboard_hook.hpp"

#include "platform/windows/input_tag.hpp"
#include "platform/windows/input_threads.hpp"

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

bool KeyboardHook::Install(InputThreads& receiver)
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

    policy_ = HotkeyPolicy(GetAsyncKeyState(VK_F13) < 0);
    receiver_ = &receiver;
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
    return true;
}

DWORD KeyboardHook::LastErrorCode() const noexcept
{
    return last_error_code_;
}

LRESULT CALLBACK KeyboardHook::HandleKeyboardEvent(const int code, const WPARAM message, const LPARAM data) noexcept
{
    if (active_hook_ == nullptr)
    {
        return CallNextHookEx(nullptr, code, message, data);
    }

    return active_hook_->DispatchKeyboardEvent(code, message, data);
}

LRESULT KeyboardHook::DispatchKeyboardEvent(const int code, const WPARAM message, const LPARAM data) noexcept
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

    const auto decision =
        policy_.Handle({is_key_down ? KeyTransition::pressed : KeyTransition::released,
                        (keyboard_event->flags & LLKHF_INJECTED) != 0, keyboard_event->dwExtraInfo == F13InputTag()});
    if (decision.double_click)
        receiver_->QueueDoubleClick();
    return decision.suppress ? 1 : CallNextHookEx(handle_, code, message, data);
}
} // namespace double_click_hotkey::windows
