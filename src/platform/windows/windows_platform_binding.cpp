#include "platform/windows/windows_platform_binding.hpp"

#include "platform/windows/console_control_handler.hpp"
#include "platform/windows/keyboard_hook.hpp"
#include "platform/windows/single_instance.hpp"

#include <windows.h>

#include <chrono>
#include <memory>
#include <sstream>
#include <utility>

namespace double_click_hotkey::windows
{
namespace
{
constexpr wchar_t SingleInstanceName[] = L"double-click-hotkey-mutex-wzyids6rnh94128qrg5t";
} // namespace

PlatformResult WindowsPlatformBinding::RunService(HotkeyEventHandler hotkey_handler,
                                                  WindowVisibilityHandler visibility_handler)
{
    const SingleInstance single_instance(SingleInstanceName);
    if (single_instance.Status() == SingleInstanceStatus::already_running)
    {
        return {PlatformResultStatus::already_running, {}};
    }
    if (single_instance.Status() == SingleInstanceStatus::failed)
    {
        return {PlatformResultStatus::failure,
                FormatError("Failed to create the single-instance mutex", single_instance.LastErrorCode())};
    }

    InstanceCommandReceiver command_receiver;
    if (!command_receiver.Initialize())
    {
        return {PlatformResultStatus::failure,
                FormatError("Failed to initialize the instance command receiver", command_receiver.LastErrorCode())};
    }

    return RunMessageLoop(std::move(hotkey_handler), std::move(visibility_handler), command_receiver);
}

PlatformResult WindowsPlatformBinding::ReserveSingleInstance()
{
    if (single_instance_reservation_ != nullptr)
    {
        return {PlatformResultStatus::failure, "The single application instance is already reserved."};
    }

    auto reservation = std::make_unique<SingleInstance>(SingleInstanceName);
    if (reservation->Status() == SingleInstanceStatus::already_running)
    {
        return {PlatformResultStatus::already_running, {}};
    }
    if (reservation->Status() == SingleInstanceStatus::failed)
    {
        return {PlatformResultStatus::failure,
                FormatError("Failed to create the single-instance mutex", reservation->LastErrorCode())};
    }

    single_instance_reservation_ = std::move(reservation);
    return {};
}

PlatformResult WindowsPlatformBinding::SendWindowCommand(const WindowVisibility visibility)
{
    DWORD error_code = ERROR_SUCCESS;
    if (!SendInstanceCommand(ToInstanceCommand(visibility), error_code))
    {
        if (error_code == ERROR_FILE_NOT_FOUND)
        {
            return {PlatformResultStatus::not_running, {}};
        }

        return {PlatformResultStatus::failure,
                FormatError("Failed to send the command to the running instance", error_code)};
    }

    return {};
}

void WindowsPlatformBinding::SetWindowVisibility(const WindowVisibility visibility)
{
    if (visibility == WindowVisibility::shown)
    {
        console_.Show();
    }
    else
    {
        console_.Hide();
    }
}

void WindowsPlatformBinding::WriteLine(const std::string_view message)
{
    console_.WriteLine(message);
}

void WindowsPlatformBinding::WaitFor(const std::chrono::milliseconds duration)
{
    Sleep(static_cast<DWORD>(duration.count()));
}

void WindowsPlatformBinding::WaitForKey()
{
    console_.WaitForKey();
}

PlatformResult WindowsPlatformBinding::SendF13()
{
    if (!keyboard_sender_.SendF13())
    {
        return {PlatformResultStatus::failure, FormatError("Failed to send F13", keyboard_sender_.LastErrorCode())};
    }

    return {};
}

PlatformResult WindowsPlatformBinding::DoubleClick()
{
    if (!mouse_.DoubleClick())
    {
        return {PlatformResultStatus::failure, FormatError("Failed to send a double-click", mouse_.LastErrorCode())};
    }

    return {};
}

std::string WindowsPlatformBinding::FormatError(const char* const message, const unsigned long error_code)
{
    std::ostringstream output;
    output << message << ", error code: " << error_code;
    return output.str();
}

InstanceCommand WindowsPlatformBinding::ToInstanceCommand(const WindowVisibility visibility) noexcept
{
    return visibility == WindowVisibility::shown ? InstanceCommand::show_window : InstanceCommand::hide_window;
}

PlatformResult WindowsPlatformBinding::RunMessageLoop(HotkeyEventHandler hotkey_handler,
                                                      WindowVisibilityHandler visibility_handler,
                                                      InstanceCommandReceiver& command_receiver)
{
    ConsoleControlHandler control_handler;
    if (!control_handler.Install())
    {
        return {PlatformResultStatus::failure,
                FormatError("Failed to set console control handler", control_handler.LastErrorCode())};
    }

    KeyboardHook keyboard_hook;
    if (!keyboard_hook.Install(std::move(hotkey_handler)))
    {
        return {PlatformResultStatus::failure,
                FormatError("Failed to set keyboard hook", keyboard_hook.LastErrorCode())};
    }

    const InstanceCommandReceiver::Handles& event_handles = command_receiver.EventHandles();
    while (true)
    {
        const DWORD result = MsgWaitForMultipleObjectsEx(static_cast<DWORD>(event_handles.size()), event_handles.data(),
                                                         INFINITE, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
        if (keyboard_hook.EventQueueFailed())
        {
            return {PlatformResultStatus::failure,
                    FormatError("Failed to queue a hotkey event", keyboard_hook.LastErrorCode())};
        }

        const DWORD first_event_result = WAIT_OBJECT_0;
        const DWORD message_result = first_event_result + static_cast<DWORD>(event_handles.size());
        if (result >= first_event_result && result < message_result)
        {
            const std::optional<InstanceCommand> command = command_receiver.TakeLatestCommand();
            if (command.has_value())
            {
                visibility_handler(*command == InstanceCommand::show_window ? WindowVisibility::shown
                                                                            : WindowVisibility::hidden);
            }
            continue;
        }
        if (result == WAIT_FAILED)
        {
            return {PlatformResultStatus::failure,
                    FormatError("Failed to wait for a message or instance command", GetLastError())};
        }
        if (result != message_result)
        {
            return {PlatformResultStatus::failure, "Received an unexpected message wait result."};
        }

        MSG message{};
        while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
        {
            if (keyboard_hook.EventQueueFailed())
            {
                return {PlatformResultStatus::failure,
                        FormatError("Failed to queue a hotkey event", keyboard_hook.LastErrorCode())};
            }
            if (message.message == WM_QUIT)
            {
                return {};
            }
            if (keyboard_hook.HandleQueuedEvent(message))
            {
                continue;
            }
            static_cast<void>(TranslateMessage(&message));
            static_cast<void>(DispatchMessageW(&message));
        }

        if (keyboard_hook.EventQueueFailed())
        {
            return {PlatformResultStatus::failure,
                    FormatError("Failed to queue a hotkey event", keyboard_hook.LastErrorCode())};
        }
    }
}
} // namespace double_click_hotkey::windows
