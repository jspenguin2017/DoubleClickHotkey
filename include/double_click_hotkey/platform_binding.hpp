#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>

namespace double_click_hotkey
{
using ElapsedTime = std::chrono::milliseconds;

enum class KeyTransition
{
    pressed,
    released
};

enum class EventKind
{
    initialized,
    show,
    hide,
    hotkey_pressed,
    hotkey_released,
    delay_changed,
    send_requested,
    tick,
    diagnostic,
    quit
};
struct ApplicationEvent
{
    EventKind kind;
    std::string text;
};
using EventHandler = std::function<void(const ApplicationEvent&)>;

struct ViewState
{
    std::string log_text;
    std::uint64_t log_revision = 0;
    std::uint64_t removed_lines = 0;
    std::string delay_text = "5";
    std::string send_caption = "Send F13";
    bool delay_enabled = true;
    bool send_enabled = true;
};

struct PlatformResult
{
    bool success = true;
    std::string error;
};

class PlatformBinding
{
  public:
    virtual ~PlatformBinding() = default;
    PlatformBinding(const PlatformBinding&) = delete;
    PlatformBinding& operator=(const PlatformBinding&) = delete;
    PlatformBinding(PlatformBinding&&) = delete;
    PlatformBinding& operator=(PlatformBinding&&) = delete;

    // Own native service resources, start hidden, and invoke callbacks only during this call. Duplicate activation
    // returns success without initialization. Deliver quit before teardown and detach all callbacks before returning.
    [[nodiscard]] virtual PlatformResult RunService(EventHandler handler) = 0;
    [[nodiscard]] virtual ElapsedTime Now() = 0;
    [[nodiscard]] virtual PlatformResult ScheduleTick(std::optional<ElapsedTime> deadline) = 0;
    virtual void Present(const ViewState& state) = 0;
    virtual void SetWindowVisible(bool visible) = 0;
    virtual void ShowError(std::string_view message) = 0;
    virtual void RequestExit() noexcept = 0;
    [[nodiscard]] virtual PlatformResult SendF13() = 0;
    [[nodiscard]] virtual PlatformResult DoubleClick() = 0;

  protected:
    PlatformBinding() = default;
};
} // namespace double_click_hotkey
