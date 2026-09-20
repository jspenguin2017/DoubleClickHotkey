#pragma once

#include "double_click_hotkey/log_buffer.hpp"
#include "double_click_hotkey/platform_binding.hpp"

#include <deque>

namespace double_click_hotkey
{
[[nodiscard]] std::optional<std::chrono::seconds> ParseDelay(std::string_view text) noexcept;

class Application
{
  public:
    explicit Application(PlatformBinding& platform);
    [[nodiscard]] int Run();

  private:
    void HandleEvent(const ApplicationEvent& event);
    void ApplyEvent(const ApplicationEvent& event);
    void WriteLine(std::string_view message);
    void UpdateCountdown();
    void Stop() noexcept;
    PlatformBinding& platform_;
    LogBuffer log_;
    ViewState view_;
    std::optional<ElapsedTime> deadline_;
    std::deque<ApplicationEvent> pending_events_;
    bool processing_ = false;
    bool stopped_ = false;
    bool hotkey_is_pressed_ = false;
};
} // namespace double_click_hotkey
