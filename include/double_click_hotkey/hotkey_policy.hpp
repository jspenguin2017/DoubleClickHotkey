#pragma once

namespace double_click_hotkey
{
enum class KeyTransition
{
    pressed,
    released
};

// Normalized F13 events only. Native filtering and input-tag comparison stay in the adapter.
struct HotkeyEvent
{
    KeyTransition transition;
    bool injected = false;
    bool matches_input_tag = false;
};

struct HotkeyDecision
{
    bool suppress = false;
    bool double_click = false;
};

// Confined to the hook thread; never calls the UI or an input sender.
class HotkeyPolicy
{
  public:
    explicit HotkeyPolicy(bool initially_pressed = false) noexcept;
    [[nodiscard]] HotkeyDecision Handle(const HotkeyEvent& event) noexcept;

  private:
    bool pressed_ = false;
    bool pass_until_release_;
};
} // namespace double_click_hotkey
