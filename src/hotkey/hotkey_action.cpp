#include "double_click_hotkey/hotkey_action.hpp"

namespace double_click_hotkey
{
HotkeyAction GetHotkeyAction(const ModifierState&) noexcept
{
    return HotkeyAction::double_click;
}
} // namespace double_click_hotkey
