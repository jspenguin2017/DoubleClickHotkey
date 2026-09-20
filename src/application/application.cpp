#include "double_click_hotkey/application.hpp"

#include <charconv>
#include <exception>
#include <utility>

namespace double_click_hotkey
{
std::optional<std::chrono::seconds> ParseDelay(const std::string_view text) noexcept
{
    if (text.empty())
        return std::nullopt;
    for (const char digit : text)
    {
        if (digit < '0' || digit > '9')
            return std::nullopt;
    }
    unsigned int seconds = 0;
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), seconds);
    if (parsed.ec != std::errc{} || seconds < 1 || seconds > 3600)
        return std::nullopt;
    return std::chrono::seconds(seconds);
}

Application::Application(PlatformBinding& platform) : platform_(platform)
{
}

int Application::Run()
{
    try
    {
        const auto result = platform_.RunService([this](const ApplicationEvent& event) { HandleEvent(event); });
        if (result.success)
            return 0;
        WriteLine(result.error);
        platform_.ShowError(result.error);
    }
    catch (const std::exception& error)
    {
        if (!stopped_)
            Stop();
        platform_.ShowError(error.what());
    }
    catch (...)
    {
        if (!stopped_)
            Stop();
        platform_.ShowError("Unexpected application error.");
    }
    return 1;
}

void Application::WriteLine(const std::string_view message)
{
    log_.AppendLine(message);
    view_.log_text = log_.Text();
    view_.removed_lines = log_.RemovedLines();
    ++view_.log_revision;
}

void Application::Stop() noexcept
{
    stopped_ = true;
    deadline_.reset();
    pending_events_.clear();
    // Native RequestExit cancels timers immediately, including during session shutdown in a modal menu loop.
    platform_.RequestExit();
}

void Application::HandleEvent(const ApplicationEvent& event)
{
    if (stopped_)
        return;
    if (event.kind == EventKind::quit)
    {
        Stop();
        return;
    }
    pending_events_.push_back(event);
    if (processing_)
        return;
    processing_ = true;
    try
    {
        while (!pending_events_.empty() && !stopped_)
        {
            const auto next = std::move(pending_events_.front());
            pending_events_.pop_front();
            ApplyEvent(next);
            if (!stopped_)
                platform_.Present(view_);
        }
        processing_ = false;
    }
    catch (...)
    {
        processing_ = false;
        Stop();
        throw;
    }
}

void Application::ApplyEvent(const ApplicationEvent& event)
{
    switch (event.kind)
    {
    case EventKind::initialized:
        platform_.SetWindowVisible(false);
        break;
    case EventKind::show:
        platform_.SetWindowVisible(true);
        break;
    case EventKind::hide:
        platform_.SetWindowVisible(false);
        break;
    case EventKind::diagnostic:
        WriteLine(event.text);
        break;
    case EventKind::delay_changed:
        if (!deadline_)
        {
            view_.delay_text = event.text;
            view_.send_enabled = ParseDelay(event.text).has_value();
        }
        break;
    case EventKind::send_requested:
        if (!deadline_)
        {
            const auto delay = ParseDelay(view_.delay_text);
            if (delay)
            {
                deadline_ = platform_.Now() + *delay;
                view_.delay_enabled = false;
                view_.send_enabled = false;
                WriteLine("F13 will be sent in " + std::to_string(delay->count()) +
                          " seconds. Focus the target application now.");
                UpdateCountdown();
            }
        }
        break;
    case EventKind::tick:
        if (deadline_)
            UpdateCountdown();
        break;
    case EventKind::quit:
        break;
    }
}

void Application::UpdateCountdown()
{
    const auto now = platform_.Now();
    if (now >= *deadline_)
    {
        deadline_.reset(); // Consume before injecting input or processing any reentrant event.
        (void)platform_.ScheduleTick(std::nullopt);
        const auto result = platform_.SendF13();
        WriteLine(result.success ? "F13 sent." : result.error);
    }
    else
    {
        const auto seconds = std::chrono::ceil<std::chrono::seconds>(*deadline_ - now);
        view_.send_caption = "Sending in " + std::to_string(seconds.count()) + " s";
        const auto result = platform_.ScheduleTick(*deadline_ - (seconds - std::chrono::seconds(1)));
        if (result.success)
            return;
        deadline_.reset();
        (void)platform_.ScheduleTick(std::nullopt);
        WriteLine(result.error);
    }
    view_.send_caption = "Send F13";
    view_.delay_enabled = true;
    view_.send_enabled = ParseDelay(view_.delay_text).has_value();
}
} // namespace double_click_hotkey
