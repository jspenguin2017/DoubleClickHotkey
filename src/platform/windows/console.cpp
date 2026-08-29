#include "platform/windows/console.hpp"

#include <conio.h>
#include <windows.h>

#include <iostream>

namespace double_click_hotkey::windows
{
bool Console::IsVisible() const noexcept
{
    return IsWindowVisible(GetConsoleWindow()) != 0;
}

void Console::Hide() noexcept
{
    ShowWindow(GetConsoleWindow(), SW_HIDE);
}

void Console::Show() noexcept
{
    ShowWindow(GetConsoleWindow(), SW_SHOW);
}

void Console::ToggleVisibility() noexcept
{
    if (IsVisible())
    {
        Hide();
    }
    else
    {
        Show();
    }
}

void Console::ReportError(const std::string_view message)
{
    Show();
    std::cout << message << std::endl;
    WaitForKey();
}

void Console::WaitForKey()
{
    std::cout << "Press any key to continue...";
    static_cast<void>(_getch());
}
} // namespace double_click_hotkey::windows
