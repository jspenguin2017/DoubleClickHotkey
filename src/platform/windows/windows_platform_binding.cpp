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
constexpr char UsageMessage[] = "Usage: DoubleClickHotkey.exe [--show | --hide | --send-f13]";
} // namespace

WindowsPlatformBinding::WindowsPlatformBinding(const bool hide_window_immediately) noexcept
{
    if (hide_window_immediately)
    {
        console_.Hide();
    }
}

int WindowsPlatformBinding::Run(const LaunchCommand launch_command, HotkeyEventHandler handler)
{
    switch (launch_command)
    {
    case LaunchCommand::run:
        return RunApplication(std::move(handler));

    case LaunchCommand::show_window:
        return SendConsoleCommand(InstanceCommand::show_window);

    case LaunchCommand::hide_window:
        return SendConsoleCommand(InstanceCommand::hide_window);

    case LaunchCommand::send_f13:
        return SendF13AfterDelay();

    case LaunchCommand::invalid:
        console_.ReportError(UsageMessage, false);
        return 1;
    }

    console_.ReportError(UsageMessage, false);
    return 1;
}

int WindowsPlatformBinding::RunApplication(HotkeyEventHandler handler)
{
    InstanceCommandReceiver command_receiver;
    if (!command_receiver.Initialize())
    {
        console_.ReportError(
            FormatError("Failed to create the instance command events", command_receiver.LastErrorCode()));
        return 1;
    }

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

    const std::string error = RunMessageLoop(std::move(handler), command_receiver);
    if (!error.empty())
    {
        console_.ReportError(error);
        return 1;
    }

    return 0;
}

int WindowsPlatformBinding::SendConsoleCommand(const InstanceCommand command)
{
    const SingleInstance single_instance(SingleInstanceName);
    if (single_instance.Status() == SingleInstanceStatus::acquired)
    {
        console_.ReportError("No running instance of this application was found.", false);
        return 1;
    }
    if (single_instance.Status() == SingleInstanceStatus::failed)
    {
        console_.ReportError(FormatError("Failed to open the single-instance mutex", single_instance.LastErrorCode()),
                             false);
        return 1;
    }

    DWORD error_code = ERROR_SUCCESS;
    if (!SendInstanceCommand(command, error_code))
    {
        console_.ReportError(FormatError("Failed to send the command to the running instance", error_code), false);
        return 1;
    }

    return 0;
}

int WindowsPlatformBinding::SendF13AfterDelay()
{
    const SingleInstance single_instance(SingleInstanceName);
    if (single_instance.Status() == SingleInstanceStatus::already_running)
    {
        console_.ReportError("Another instance of this application is already running. Close it before sending F13.",
                             false);
        return 1;
    }
    if (single_instance.Status() == SingleInstanceStatus::failed)
    {
        console_.ReportError(FormatError("Failed to create the single-instance mutex", single_instance.LastErrorCode()),
                             false);
        return 1;
    }

    console_.WriteLine("F13 will be sent in 5 seconds. Focus the target application now.");
    Sleep(5000);
    if (!keyboard_sender_.SendF13())
    {
        console_.ReportError(FormatError("Failed to send F13", keyboard_sender_.LastErrorCode()), false);
        return 1;
    }

    return 0;
}

void WindowsPlatformBinding::DoubleClick()
{
    mouse_.DoubleClick();
}

std::string WindowsPlatformBinding::FormatError(const char* const message, const unsigned long error_code)
{
    std::ostringstream output;
    output << message << ", error code: " << error_code;
    return output.str();
}

std::string WindowsPlatformBinding::RunMessageLoop(HotkeyEventHandler handler,
                                                   const InstanceCommandReceiver& command_receiver)
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

    const InstanceCommandReceiver::Handles& event_handles = command_receiver.EventHandles();
    while (true)
    {
        const DWORD result = MsgWaitForMultipleObjectsEx(static_cast<DWORD>(event_handles.size()), event_handles.data(),
                                                         INFINITE, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
        const DWORD first_event_result = WAIT_OBJECT_0;
        const DWORD message_result = first_event_result + static_cast<DWORD>(event_handles.size());
        if (result >= first_event_result && result < message_result)
        {
            const InstanceCommand command = command_receiver.CommandAt(result - first_event_result);
            if (command == InstanceCommand::show_window)
            {
                console_.Show();
            }
            else
            {
                console_.Hide();
            }
            continue;
        }
        if (result == WAIT_FAILED)
        {
            return FormatError("Failed to wait for a message or instance command", GetLastError());
        }
        if (result != message_result)
        {
            return "Received an unexpected message wait result.";
        }

        MSG message{};
        while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
        {
            if (message.message == WM_QUIT)
            {
                return {};
            }
            static_cast<void>(TranslateMessage(&message));
            static_cast<void>(DispatchMessageW(&message));
        }
    }
}
} // namespace double_click_hotkey::windows
