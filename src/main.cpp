#include "double_click_hotkey/hotkey_action.hpp"

#include <conio.h>
#include <windows.h>

#include <cstdlib>
#include <iostream>

namespace
{
HHOOK KeyboardHookHandle = nullptr;

void WaitKey()
{
    std::cout << "Press any key to continue...";
    static_cast<void>(_getch());
}

[[noreturn]] void CleanUpExit(int exit_code = 0)
{
    if (KeyboardHookHandle)
    {
        UnhookWindowsHookEx(KeyboardHookHandle);
        KeyboardHookHandle = nullptr;
    }

    std::exit(exit_code);
}

bool IsConsoleVisible()
{
    return IsWindowVisible(GetConsoleWindow()) != 0;
}

void HideConsole()
{
    ShowWindow(GetConsoleWindow(), SW_HIDE);
}

void ShowConsole()
{
    ShowWindow(GetConsoleWindow(), SW_SHOW);
}

BOOL WINAPI ConsoleCtrlHandler(DWORD)
{
    CleanUpExit();
}

LRESULT CALLBACK KeyboardHookHandler(int code, WPARAM message, LPARAM data)
{
    if (code < 0)
    {
        return CallNextHookEx(nullptr, code, message, data);
    }

    const bool is_key_down = message == WM_KEYDOWN || message == WM_SYSKEYDOWN;
    const bool is_key_up = message == WM_KEYUP || message == WM_SYSKEYUP;

    if (is_key_down || is_key_up)
    {
        const auto* keyboard_event = reinterpret_cast<PKBDLLHOOKSTRUCT>(data);

        if (keyboard_event->vkCode == VK_F8)
        {
            const double_click_hotkey::ModifierState modifiers{
                GetKeyState(VK_MENU) < 0,
                GetKeyState(VK_CONTROL) < 0,
                GetKeyState(VK_SHIFT) < 0,
                GetKeyState(VK_LWIN) < 0,
                GetKeyState(VK_RWIN) < 0,
            };

            switch (double_click_hotkey::GetHotkeyAction(modifiers))
            {
            case double_click_hotkey::HotkeyAction::toggle_console:
                if (is_key_down)
                {
                    if (IsConsoleVisible())
                    {
                        HideConsole();
                    }
                    else
                    {
                        ShowConsole();
                    }
                }
                return 1;

            case double_click_hotkey::HotkeyAction::double_click:
                if (is_key_down)
                {
                    INPUT input{};
                    input.type = INPUT_MOUSE;

                    input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
                    SendInput(1, &input, sizeof(input));
                    input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
                    SendInput(1, &input, sizeof(input));

                    input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
                    SendInput(1, &input, sizeof(input));
                    input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
                    SendInput(1, &input, sizeof(input));
                }
                return 1;

            case double_click_hotkey::HotkeyAction::none:
                break;
            }
        }
    }

    return CallNextHookEx(nullptr, code, message, data);
}
} // namespace

int main()
{
    HideConsole();

    const HANDLE single_instance_mutex =
        CreateMutex(nullptr, TRUE, L"double-click-hotkey-mutex-wzyids6rnh94128qrg5t");
    if (!single_instance_mutex || GetLastError() == ERROR_ALREADY_EXISTS)
    {
        ShowConsole();
        std::cout << "Another instance of this application is already running." << std::endl;
        WaitKey();
        CleanUpExit(1);
    }

    if (!SetConsoleCtrlHandler(&ConsoleCtrlHandler, TRUE))
    {
        ShowConsole();
        std::cout << "Failed to set console ctrl handler, error code: " << GetLastError() << std::endl;
        WaitKey();
        CleanUpExit(1);
    }

    KeyboardHookHandle = SetWindowsHookEx(WH_KEYBOARD_LL, &KeyboardHookHandler, nullptr, 0);
    if (!KeyboardHookHandle)
    {
        ShowConsole();
        std::cout << "Failed to set keyboard hook, error code: " << GetLastError() << std::endl;
        WaitKey();
        CleanUpExit(1);
    }

    MSG message{};
    do
    {
        const BOOL result = GetMessage(&message, nullptr, 0, 0);

        if (result == -1)
        {
            ShowConsole();
            std::cout << "Failed to get message." << std::endl;
            WaitKey();
            CleanUpExit(1);
        }

        if (result == 0)
        {
            CleanUpExit();
        }
    } while (true);
}
