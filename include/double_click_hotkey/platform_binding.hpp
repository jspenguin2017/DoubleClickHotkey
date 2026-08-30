#pragma once

#include "double_click_hotkey/hotkey_action.hpp"
#include "double_click_hotkey/launch_command.hpp"

#include <functional>

namespace double_click_hotkey
{
enum class KeyTransition
{
    pressed,
    released,
};

struct HotkeyEvent
{
    KeyTransition transition = KeyTransition::pressed;
    ModifierState modifiers;
};

// Returning true tells the binding to suppress the native hotkey transition.
using HotkeyEventHandler = std::function<bool(const HotkeyEvent&)>;

class PlatformBinding
{
  public:
    virtual ~PlatformBinding() = default;

    PlatformBinding(const PlatformBinding&) = delete;
    PlatformBinding& operator=(const PlatformBinding&) = delete;
    PlatformBinding(PlatformBinding&&) = delete;
    PlatformBinding& operator=(PlatformBinding&&) = delete;

    // The binding owns its native lifecycle and invokes the handler only while the normal run command is active.
    [[nodiscard]] virtual int Run(LaunchCommand launch_command, HotkeyEventHandler handler) = 0;
    virtual void DoubleClick() = 0;

  protected:
    PlatformBinding() = default;
};
} // namespace double_click_hotkey
