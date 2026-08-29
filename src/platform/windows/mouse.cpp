#include "platform/windows/mouse.hpp"

#include <windows.h>

#include <array>

namespace double_click_hotkey::windows
{
void Mouse::DoubleClick() noexcept
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

    static_cast<void>(SendInput(static_cast<UINT>(inputs.size()), inputs.data(), sizeof(INPUT)));
}
} // namespace double_click_hotkey::windows
