#include "platform/windows/clock.hpp"

#include <windows.h>

namespace double_click_hotkey::windows
{
ElapsedTime InterruptTime() noexcept
{
    ULONGLONG ticks = 0;
    QueryInterruptTime(&ticks);
    return ElapsedTime(ticks / 10000); // Includes suspend/hibernate; unaffected by wall-clock changes.
}
} // namespace double_click_hotkey::windows
