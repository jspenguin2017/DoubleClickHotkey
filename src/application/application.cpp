#include "double_click_hotkey/application.hpp"

#include "double_click_hotkey/hotkey_action.hpp"

namespace double_click_hotkey
{
Application::Application(PlatformBinding& platform, const LaunchCommand launch_command) noexcept
    : platform_(platform), launch_command_(launch_command)
{
}

int Application::Run()
{
    return platform_.Run(launch_command_, [this](const HotkeyEvent& event) { return HandleHotkeyEvent(event); });
}

bool Application::HandleHotkeyEvent(const HotkeyEvent& event)
{
    const HotkeyAction action = GetHotkeyAction(event.modifiers);
    if (action == HotkeyAction::none)
    {
        return false;
    }

    if (event.transition == KeyTransition::pressed)
    {
        switch (action)
        {
        case HotkeyAction::double_click:
            platform_.DoubleClick();
            break;

        case HotkeyAction::none:
            break;
        }
    }

    return true;
}
} // namespace double_click_hotkey
