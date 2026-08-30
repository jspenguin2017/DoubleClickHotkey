#include "platform/windows/keyboard_sender.hpp"

#include <array>

namespace double_click_hotkey::windows
{
KeyboardSender::KeyboardSender(const SendInputFunction send_input) noexcept : input_injector_(send_input)
{
}

bool KeyboardSender::SendF13() noexcept
{
    std::array<INPUT, 2> inputs{};
    for (INPUT& input : inputs)
    {
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = VK_F13;
    }
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

    return input_injector_.SendBalancedSequence(inputs.data(), static_cast<UINT>(inputs.size()));
}

DWORD KeyboardSender::LastErrorCode() const noexcept
{
    return input_injector_.LastErrorCode();
}

std::optional<DWORD> KeyboardSender::LastReleaseErrorCode() const noexcept
{
    return input_injector_.LastReleaseErrorCode();
}
} // namespace double_click_hotkey::windows
