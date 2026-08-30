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
    std::array<INPUT, 4> inputs{};
    for (INPUT& input : inputs)
    {
        input.type = INPUT_MOUSE;
    }

    inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
    inputs[2].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    inputs[3].mi.dwFlags = MOUSEEVENTF_LEFTUP;

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
