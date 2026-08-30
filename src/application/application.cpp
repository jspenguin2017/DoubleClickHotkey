#include "double_click_hotkey/application.hpp"

#include <chrono>

namespace double_click_hotkey
{
namespace
{
constexpr char UsageMessage[] = "Usage: DoubleClickHotkey [--show | --hide | --send-f13]";
constexpr char AlreadyRunningMessage[] = "Another instance of this application is already running.";
constexpr char NoRunningInstanceMessage[] = "No running instance of this application was found.";
constexpr auto SendF13Delay = std::chrono::seconds(5);
} // namespace

Application::Application(PlatformBinding& platform, const LaunchCommand launch_command) noexcept
    : platform_(platform), launch_command_(launch_command)
{
}

int Application::Run()
{
    switch (launch_command_)
    {
    case LaunchCommand::run:
        return RunService();

    case LaunchCommand::show_window:
        return SendWindowCommand(WindowVisibility::shown);

    case LaunchCommand::hide_window:
        return SendWindowCommand(WindowVisibility::hidden);

    case LaunchCommand::send_f13:
        return SendF13AfterDelay();

    case LaunchCommand::invalid:
        ReportError(UsageMessage, false);
        return 1;
    }

    ReportError(UsageMessage, false);
    return 1;
}

int Application::RunService()
{
    platform_.SetWindowVisibility(WindowVisibility::hidden);

    const PlatformResult result =
        platform_.RunService([this](const HotkeyEvent& event) { HandleHotkeyEvent(event); },
                             [this](const WindowVisibility visibility) { HandleWindowVisibility(visibility); });
    if (result.status == PlatformResultStatus::success)
    {
        return 0;
    }
    if (result.status == PlatformResultStatus::already_running)
    {
        ReportError(AlreadyRunningMessage, true);
        return 1;
    }

    ReportResultError(result, true);
    return 1;
}

int Application::SendWindowCommand(const WindowVisibility visibility)
{
    const PlatformResult result = platform_.SendWindowCommand(visibility);
    if (result.status == PlatformResultStatus::success)
    {
        return 0;
    }
    if (result.status == PlatformResultStatus::not_running)
    {
        ReportError(NoRunningInstanceMessage, false);
        return 1;
    }

    ReportResultError(result);
    return 1;
}

int Application::SendF13AfterDelay()
{
    const PlatformResult reservation = platform_.ReserveSingleInstance();
    if (reservation.status == PlatformResultStatus::already_running)
    {
        ReportError("Another instance of this application is already running. Close it before sending F13.", false);
        return 1;
    }
    if (reservation.status != PlatformResultStatus::success)
    {
        ReportResultError(reservation);
        return 1;
    }

    platform_.WriteLine("F13 will be sent in 5 seconds. Focus the target application now.");
    platform_.WaitFor(SendF13Delay);

    const PlatformResult result = platform_.SendF13();
    if (result.status != PlatformResultStatus::success)
    {
        ReportResultError(result);
        return 1;
    }

    return 0;
}

void Application::HandleHotkeyEvent(const HotkeyEvent& event)
{
    if (event.transition == KeyTransition::released)
    {
        hotkey_is_pressed_ = false;
        return;
    }

    if (hotkey_is_pressed_)
    {
        return;
    }

    hotkey_is_pressed_ = true;
    const PlatformResult result = platform_.DoubleClick();
    if (result.status == PlatformResultStatus::failure)
    {
        platform_.WriteLine(result.error_message);
    }
}

void Application::HandleWindowVisibility(const WindowVisibility visibility)
{
    platform_.SetWindowVisibility(visibility);
}

void Application::ReportError(const std::string_view message, const bool wait_for_key)
{
    platform_.SetWindowVisibility(WindowVisibility::shown);
    platform_.WriteLine(message);
    if (wait_for_key)
    {
        platform_.WaitForKey();
    }
}

void Application::ReportResultError(const PlatformResult& result, const bool wait_for_key)
{
    if (result.status == PlatformResultStatus::failure)
    {
        ReportError(result.error_message, wait_for_key);
    }
}
} // namespace double_click_hotkey
