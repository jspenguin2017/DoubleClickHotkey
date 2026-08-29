#pragma once

namespace double_click_hotkey
{
struct ModifierState
{
    bool alt = false;
    bool control = false;
    bool shift = false;
    bool left_windows = false;
    bool right_windows = false;
};

enum class HotkeyAction
{
    none,
    double_click,
    toggle_console,
};

[[nodiscard]] HotkeyAction GetHotkeyAction(const ModifierState& modifiers) noexcept;
} // namespace double_click_hotkey
