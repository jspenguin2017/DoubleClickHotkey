#include "double_click_hotkey/application.hpp"

#include "double_click_hotkey/hotkey_action.hpp"

namespace double_click_hotkey
{
Application::Application(PlatformBinding& platform) noexcept : platform_(platform)
{
}

int Application::Run()
{
    return platform_.Run([this](const HotkeyEvent& event) { return HandleHotkeyEvent(event); });
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

        case HotkeyAction::toggle_console:
            platform_.ToggleConsoleVisibility();
            break;

        case HotkeyAction::none:
            break;
        }
    }

    return true;
}
} // namespace double_click_hotkey
