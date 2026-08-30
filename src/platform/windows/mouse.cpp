#include "platform/windows/mouse.hpp"

#include <windows.h>

#include <array>

namespace double_click_hotkey::windows
{
bool Mouse::DoubleClick() noexcept
{
    std::array<INPUT, 4> inputs{};
    for (INPUT& input : inputs)
    {
        input.type = INPUT_MOUSE;
    }

    inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
    inputs[2].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    inputs[3].mi.dwFlags = MOUSEEVENTF_LEFTUP;

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

DWORD Mouse::LastErrorCode() const noexcept
{
    return last_error_code_;
}
} // namespace double_click_hotkey::windows
