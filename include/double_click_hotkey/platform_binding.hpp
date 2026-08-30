#pragma once

#include <chrono>
#include <functional>
#include <string>
#include <string_view>

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
};

// Returning true tells the binding to suppress the native hotkey transition.
using HotkeyEventHandler = std::function<bool(const HotkeyEvent&)>;

enum class WindowVisibility
{
    hidden,
    shown,
};

using WindowVisibilityHandler = std::function<void(WindowVisibility)>;

enum class PlatformResultStatus
{
    success,
    already_running,
    not_running,
    failure,
};

struct PlatformResult
{
    PlatformResultStatus status = PlatformResultStatus::success;
    // Set when status is failure; the message describes the native operation that failed.
    std::string error_message;
};

class PlatformBinding
{
  public:
    virtual ~PlatformBinding() = default;

    PlatformBinding(const PlatformBinding&) = delete;
    PlatformBinding& operator=(const PlatformBinding&) = delete;
    PlatformBinding(PlatformBinding&&) = delete;
    PlatformBinding& operator=(PlatformBinding&&) = delete;

    // RunService returns success after a normal stop, already_running when another service owns the instance, or
    // failure for a native setup or event-loop error. The binding owns native service resources and invokes handlers
    // only while this call is active.
    [[nodiscard]] virtual PlatformResult RunService(HotkeyEventHandler hotkey_handler,
                                                    WindowVisibilityHandler visibility_handler) = 0;

    // ReserveSingleInstance returns success, already_running, or failure. A successful reservation is held until the
    // binding is destroyed. SendWindowCommand returns success, not_running, or failure.
    [[nodiscard]] virtual PlatformResult ReserveSingleInstance() = 0;
    [[nodiscard]] virtual PlatformResult SendWindowCommand(WindowVisibility visibility) = 0;

    virtual void SetWindowVisibility(WindowVisibility visibility) = 0;
    virtual void WriteLine(std::string_view message) = 0;
    virtual void WaitFor(std::chrono::milliseconds duration) = 0;
    virtual void WaitForKey() = 0;

    [[nodiscard]] virtual PlatformResult SendF13() = 0;
    [[nodiscard]] virtual PlatformResult DoubleClick() = 0;

  protected:
    PlatformBinding() = default;
};
} // namespace double_click_hotkey
