#include "platform/windows/instance_command.hpp"

namespace double_click_hotkey::windows
{
namespace
{
constexpr wchar_t ShowWindowEventName[] = L"double-click-hotkey-show-event-wzyids6rnh94128qrg5t";
constexpr wchar_t HideWindowEventName[] = L"double-click-hotkey-hide-event-wzyids6rnh94128qrg5t";

const wchar_t* EventName(const InstanceCommand command) noexcept
{
    return command == InstanceCommand::show_window ? ShowWindowEventName : HideWindowEventName;
}
} // namespace

InstanceCommandReceiver::~InstanceCommandReceiver()
{
    for (const HANDLE event_handle : event_handles_)
    {
        if (event_handle != nullptr)
        {
            static_cast<void>(CloseHandle(event_handle));
        }
    }
}

bool InstanceCommandReceiver::Initialize() noexcept
{
    if (event_handles_[0] != nullptr || event_handles_[1] != nullptr)
    {
        last_error_code_ = ERROR_ALREADY_EXISTS;
        return false;
    }

    event_handles_[0] = CreateEventW(nullptr, FALSE, FALSE, ShowWindowEventName);
    if (event_handles_[0] == nullptr)
    {
        last_error_code_ = GetLastError();
        return false;
    }

    event_handles_[1] = CreateEventW(nullptr, FALSE, FALSE, HideWindowEventName);
    if (event_handles_[1] == nullptr)
    {
        last_error_code_ = GetLastError();
        static_cast<void>(CloseHandle(event_handles_[0]));
        event_handles_[0] = nullptr;
        return false;
    }

    last_error_code_ = ERROR_SUCCESS;
    return true;
}

const InstanceCommandReceiver::Handles& InstanceCommandReceiver::EventHandles() const noexcept
{
    return event_handles_;
}

InstanceCommand InstanceCommandReceiver::CommandAt(const std::size_t index) const noexcept
{
    return index == 0 ? InstanceCommand::show_window : InstanceCommand::hide_window;
}

DWORD InstanceCommandReceiver::LastErrorCode() const noexcept
{
    return last_error_code_;
}

bool SendInstanceCommand(const InstanceCommand command, DWORD& error_code) noexcept
{
    const HANDLE event_handle = OpenEventW(EVENT_MODIFY_STATE, FALSE, EventName(command));
    if (event_handle == nullptr)
    {
        error_code = GetLastError();
        return false;
    }

    const bool succeeded = SetEvent(event_handle) != 0;
    error_code = succeeded ? ERROR_SUCCESS : GetLastError();
    static_cast<void>(CloseHandle(event_handle));
    return succeeded;
}
} // namespace double_click_hotkey::windows
