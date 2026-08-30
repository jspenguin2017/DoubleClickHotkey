#include "double_click_hotkey/hotkey_action.hpp"

#include <gtest/gtest.h>

namespace double_click_hotkey
{
TEST(HotkeyActionTest, DoubleClicksWithoutModifiers)
{
    EXPECT_EQ(GetHotkeyAction({}), HotkeyAction::double_click);
}

TEST(HotkeyActionTest, DoubleClicksWithEveryModifierCombination)
{
    for (unsigned int modifier_mask = 0; modifier_mask < 16; ++modifier_mask)
    {
        SCOPED_TRACE(modifier_mask);
        ModifierState modifiers;
        modifiers.alt = (modifier_mask & 1U) != 0;
        modifiers.control = (modifier_mask & 2U) != 0;
        modifiers.shift = (modifier_mask & 4U) != 0;
        modifiers.system = (modifier_mask & 8U) != 0;

        EXPECT_EQ(GetHotkeyAction(modifiers), HotkeyAction::double_click);
    }
}
} // namespace double_click_hotkey
