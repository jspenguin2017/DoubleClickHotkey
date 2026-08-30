#include "platform/windows/keyboard_sender.hpp"

#include <array>

namespace double_click_hotkey::windows
{
bool KeyboardSender::SendF13() noexcept
{
    std::array<INPUT, 2> inputs{};
    for (INPUT& input : inputs)
    {
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = VK_F13;
    }
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

    const UINT input_count = static_cast<UINT>(inputs.size());
    SetLastError(ERROR_SUCCESS);
    const UINT sent_input_count = SendInput(input_count, inputs.data(), sizeof(INPUT));
    if (sent_input_count != input_count)
    {
        last_error_code_ = GetLastError();
        return false;
    }

    last_error_code_ = ERROR_SUCCESS;
    return true;
}

DWORD KeyboardSender::LastErrorCode() const noexcept
{
    return last_error_code_;
}
} // namespace double_click_hotkey::windows
