#pragma once

#include "double_click_hotkey/platform_binding.hpp"

#include <utility>
#include <vector>

namespace double_click_hotkey
{
class FakePlatformBinding : public PlatformBinding
{
  public:
    PlatformResult RunService(EventHandler handler) override
    {
        ++run_count;
        if (!service_result.success || duplicate)
            return service_result;
        handler_ = std::move(handler);
        try
        {
            Emit(EventKind::initialized);
            if (run_action)
                run_action(*this);
            if (!service_result.success)
                Emit(EventKind::diagnostic, service_result.error);
            Emit(EventKind::quit);
        }
        catch (...)
        {
            Emit(EventKind::quit);
            handler_ = {};
            throw;
        }
        handler_ = {};
        return service_result;
    }
    void Emit(EventKind kind, std::string text = {})
    {
        if (handler_)
            handler_({kind, std::move(text)});
    }
    ElapsedTime Now() override
    {
        return now;
    }
    PlatformResult ScheduleTick(std::optional<ElapsedTime> deadline) override
    {
        scheduled.push_back(deadline);
        if (deadline && !timer_result.success)
            return timer_result;
        timer = deadline;
        return {};
    }
    void Present(const ViewState& state) override
    {
        view = state;
        ++present_count;
        if (on_present)
            on_present(*this);
    }
    void SetWindowVisible(bool visible) override
    {
        visibility.push_back(visible);
    }
    void ShowError(std::string_view message) override
    {
        errors.emplace_back(message);
    }
    void RequestExit() noexcept override
    {
        ++exit_count;
        timer.reset();
    }
    PlatformResult SendF13() override
    {
        ++send_count;
        return send_result;
    }

    EventHandler handler_;
    std::function<void(FakePlatformBinding&)> run_action;
    std::function<void(FakePlatformBinding&)> on_present;
    PlatformResult service_result, timer_result, send_result;
    ElapsedTime now{0};
    std::optional<ElapsedTime> timer;
    std::vector<std::optional<ElapsedTime>> scheduled;
    ViewState view;
    std::vector<bool> visibility;
    std::vector<std::string> errors;
    bool duplicate = false;
    int run_count = 0, present_count = 0, exit_count = 0, send_count = 0;
};
} // namespace double_click_hotkey
