#include "double_click_hotkey/hotkey_action.hpp"

#include <gtest/gtest.h>

namespace double_click_hotkey
{
TEST(HotkeyActionTest, DoubleClicksWithoutModifiers)
{
    EXPECT_EQ(GetHotkeyAction({}), HotkeyAction::double_click);
}

TEST(HotkeyActionTest, TogglesConsoleWithAltControlAndShift)
{
    ModifierState modifiers;
    modifiers.alt = true;
    modifiers.control = true;
    modifiers.shift = true;

    EXPECT_EQ(GetHotkeyAction(modifiers), HotkeyAction::toggle_console);
}

TEST(HotkeyActionTest, TogglesConsoleWhenTheSystemModifierIsAlsoPressed)
{
    ModifierState modifiers;
    modifiers.alt = true;
    modifiers.control = true;
    modifiers.shift = true;
    modifiers.system = true;

    EXPECT_EQ(GetHotkeyAction(modifiers), HotkeyAction::toggle_console);
}

TEST(HotkeyActionTest, DoesNothingWithAnIncompleteModifierCombination)
{
    ModifierState modifiers;
    modifiers.control = true;
    modifiers.shift = true;

    EXPECT_EQ(GetHotkeyAction(modifiers), HotkeyAction::none);
}

TEST(HotkeyActionTest, DoesNothingWithTheSystemModifier)
{
    ModifierState modifiers;
    modifiers.system = true;

    EXPECT_EQ(GetHotkeyAction(modifiers), HotkeyAction::none);
}
} // namespace double_click_hotkey
