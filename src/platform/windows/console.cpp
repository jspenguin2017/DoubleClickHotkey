#include "platform/windows/console.hpp"

#include <conio.h>
#include <windows.h>

#include <cstdio>

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
    if (!message.empty())
    {
        static_cast<void>(std::fwrite(message.data(), 1, message.size(), stdout));
    }
    static_cast<void>(std::fputc('\n', stdout));
    static_cast<void>(std::fflush(stdout));
}

void Console::WaitForKey()
{
    static_cast<void>(std::fputs("Press any key to continue...", stdout));
    static_cast<void>(std::fflush(stdout));
    static_cast<void>(_getch());
}
} // namespace double_click_hotkey::windows
