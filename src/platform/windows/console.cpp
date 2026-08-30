#include "platform/windows/console.hpp"

#include <conio.h>
#include <windows.h>

#include <iostream>

namespace double_click_hotkey::windows
{
void Console::Hide() noexcept
{
    ShowWindow(GetConsoleWindow(), SW_HIDE);
}

void Console::Show() noexcept
{
    ShowWindow(GetConsoleWindow(), SW_SHOW);
}

void Console::WriteLine(const std::string_view message)
{
    std::cout << message << std::endl;
}

void Console::ReportError(const std::string_view message, const bool wait_for_key)
{
    Show();
    WriteLine(message);
    if (wait_for_key)
    {
        WaitForKey();
    }
}

void Console::WaitForKey()
{
    std::cout << "Press any key to continue...";
    static_cast<void>(_getch());
}
} // namespace double_click_hotkey::windows
