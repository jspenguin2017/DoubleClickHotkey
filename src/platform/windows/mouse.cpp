#include "platform/windows/mouse.hpp"

#include <windows.h>

#include <array>

namespace double_click_hotkey::windows
{
Mouse::Mouse(const SendInputFunction send_input) noexcept : input_injector_(send_input)
{
}

bool Mouse::DoubleClick() noexcept
{
    const bool buttons_swapped = GetSystemMetrics(SM_SWAPBUTTON) != 0;
    const DWORD button_down_flag = buttons_swapped ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_LEFTDOWN;
    const DWORD button_up_flag = buttons_swapped ? MOUSEEVENTF_RIGHTUP : MOUSEEVENTF_LEFTUP;

    std::array<INPUT, 4> inputs{};
    for (INPUT& input : inputs)
    {
        input.type = INPUT_MOUSE;
    }

    inputs[0].mi.dwFlags = button_down_flag;
    inputs[1].mi.dwFlags = button_up_flag;
    inputs[2].mi.dwFlags = button_down_flag;
    inputs[3].mi.dwFlags = button_up_flag;

    return input_injector_.SendBalancedSequence(inputs.data(), static_cast<UINT>(inputs.size()));
}

DWORD Mouse::LastErrorCode() const noexcept
{
    return input_injector_.LastErrorCode();
}

std::optional<DWORD> Mouse::LastReleaseErrorCode() const noexcept
{
    return input_injector_.LastReleaseErrorCode();
}
} // namespace double_click_hotkey::windows
