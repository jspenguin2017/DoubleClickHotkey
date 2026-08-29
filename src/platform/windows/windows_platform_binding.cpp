#include "platform/windows/windows_platform_binding.hpp"

#include "platform/windows/console_control_handler.hpp"
#include "platform/windows/keyboard_hook.hpp"
#include "platform/windows/single_instance.hpp"

#include <windows.h>

#include <sstream>
#include <utility>

namespace double_click_hotkey::windows
{
namespace
{
constexpr wchar_t SingleInstanceName[] = L"double-click-hotkey-mutex-wzyids6rnh94128qrg5t";
}

int WindowsPlatformBinding::Run(HotkeyEventHandler handler)
{
    console_.Hide();

    const SingleInstance single_instance(SingleInstanceName);
    if (single_instance.Status() == SingleInstanceStatus::already_running)
    {
        console_.ReportError("Another instance of this application is already running.");
        return 1;
    }
    if (single_instance.Status() == SingleInstanceStatus::failed)
    {
        console_.ReportError(
            FormatError("Failed to create the single-instance mutex", single_instance.LastErrorCode()));
        return 1;
    }

    const std::string error = RunMessageLoop(std::move(handler));
    if (!error.empty())
    {
        console_.ReportError(error);
        return 1;
    }

    return 0;
}

void WindowsPlatformBinding::DoubleClick()
{
    mouse_.DoubleClick();
}

void WindowsPlatformBinding::ToggleConsoleVisibility()
{
    console_.ToggleVisibility();
}

std::string WindowsPlatformBinding::FormatError(const char* const message, const unsigned long error_code)
{
    std::ostringstream output;
    output << message << ", error code: " << error_code;
    return output.str();
}

std::string WindowsPlatformBinding::RunMessageLoop(HotkeyEventHandler handler)
{
    ConsoleControlHandler control_handler;
    if (!control_handler.Install())
    {
        return FormatError("Failed to set console control handler", control_handler.LastErrorCode());
    }

    KeyboardHook keyboard_hook;
    if (!keyboard_hook.Install(std::move(handler)))
    {
        return FormatError("Failed to set keyboard hook", keyboard_hook.LastErrorCode());
    }

    MSG message{};
    while (true)
    {
        const BOOL result = GetMessageW(&message, nullptr, 0, 0);
        if (result == -1)
        {
            return "Failed to get message.";
        }
        if (result == 0)
        {
            return {};
        }
    }
}
} // namespace double_click_hotkey::windows
