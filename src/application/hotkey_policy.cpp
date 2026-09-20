#include "double_click_hotkey/hotkey_policy.hpp"

namespace double_click_hotkey
{
HotkeyPolicy::HotkeyPolicy(const bool initially_pressed) noexcept : pass_until_release_(initially_pressed)
{
}

HotkeyDecision HotkeyPolicy::Handle(const HotkeyEvent& event) noexcept
{
    // Both halves of our setup keystroke bypass all physical-key state, including startup passthrough.
    if (event.injected && event.matches_input_tag)
        return {};

    if (pass_until_release_)
    {
        if (event.transition == KeyTransition::released)
            pass_until_release_ = false;
        return {};
    }

    if (event.transition == KeyTransition::pressed)
    {
        const bool first_press = !pressed_;
        pressed_ = true;
        return {true, first_press};
    }

    const bool was_pressed = pressed_;
    pressed_ = false;
    return {was_pressed, false};
}
} // namespace double_click_hotkey
