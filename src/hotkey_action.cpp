#include "double_click_hotkey/hotkey_action.hpp"

namespace double_click_hotkey
{
HotkeyAction GetHotkeyAction(const ModifierState& modifiers) noexcept
{
    if (modifiers.alt && modifiers.control && modifiers.shift)
    {
        return HotkeyAction::toggle_console;
    }

    const bool has_modifier = modifiers.alt || modifiers.control || modifiers.shift || modifiers.left_windows ||
                              modifiers.right_windows;
    return has_modifier ? HotkeyAction::none : HotkeyAction::double_click;
}
} // namespace double_click_hotkey
