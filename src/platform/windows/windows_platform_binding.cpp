#include "platform/windows/windows_platform_binding.hpp"

#include "platform/windows/clock.hpp"
#include "platform/windows/single_instance.hpp"

namespace double_click_hotkey::windows
{
PlatformResult WindowsPlatformBinding::RunService(EventHandler handler)
{
    PlatformResult result;
    SingleInstance instance; // Keep ownership until the hook and all UI resources have been released.
    bool initialized = false;
    try
    {
        if (!instance.AcquireOrShow())
            return {};
        window_ = std::make_unique<MainWindow>(handler);
        tray_ = std::make_unique<TrayIcon>(window_->Get(), window_->SmallIcon());
        window_->SetTray(tray_.get());
        hook_ = std::make_unique<KeyboardHook>();
        if (!hook_->Install(window_->Get()))
            throw std::runtime_error(NativeError("Install F13 keyboard hook", hook_->LastErrorCode()));
        const bool tray_ready = tray_->Add();
        initialized = true;
        handler({EventKind::initialized, {}});
        if (!tray_ready)
        {
            handler(
                {EventKind::diagnostic,
                 "Unable to create the tray icon. Use Quit Double Click Hotkey in the window's system menu to exit."});
            handler({EventKind::show, {}});
        }
        instance.StartListening(); // Publish readiness only after the window, hook, and initial presentation exist.
        const HANDLE show_event = instance.ShowEvent();
        while (!exiting_)
        {
            const auto wait = MsgWaitForMultipleObjectsEx(1, &show_event, INFINITE, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
            Require(wait != WAIT_FAILED, "Wait for application events");
            if (wait == WAIT_OBJECT_0)
                handler({EventKind::show, {}});
            MSG message{};
            // Bound drains to avoid starving duplicate-instance Show requests in a busy message queue.
            for (int count = 0; count < 128 && !exiting_ && PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE); ++count)
            {
                if (message.message == WM_QUIT)
                {
                    exiting_ = true;
                    break;
                }
                if (!window_->PreTranslate(message))
                {
                    TranslateMessage(&message);
                    DispatchMessageW(&message);
                }
                if (window_->Failure())
                    std::rethrow_exception(window_->Failure());
                if (hook_ && hook_->EventQueueFailed())
                    throw std::runtime_error(NativeError("Queue F13 event", hook_->LastErrorCode()));
            }
        }
        if (window_->Failure())
            std::rethrow_exception(window_->Failure());
    }
    catch (const std::exception& error)
    {
        result = {false, error.what()};
    }
    catch (...)
    {
        result = {false, "Unexpected native application error."};
    }
    if (initialized)
    {
        try
        {
            if (!result.success)
                handler({EventKind::diagnostic, result.error});
        }
        catch (...)
        {
        }
        try
        {
            handler({EventKind::quit, {}});
        }
        catch (...)
        {
            // Native cleanup still runs if controller/presentation allocation fails.
        }
    }
    Cleanup();
    return result;
}

void WindowsPlatformBinding::Cleanup() noexcept
{
    exiting_ = true;
    if (window_)
    {
        window_->ClearHandler();
        window_->CancelTimer();
        window_->SetTray(nullptr);
    }
    hook_.reset();
    tray_.reset();
    window_.reset();
}

ElapsedTime WindowsPlatformBinding::Now()
{
    return InterruptTime();
}
PlatformResult WindowsPlatformBinding::ScheduleTick(const std::optional<ElapsedTime> deadline)
{
    return window_ ? window_->ScheduleTick(deadline) : PlatformResult{};
}
void WindowsPlatformBinding::Present(const ViewState& state)
{
    if (window_ && !exiting_)
        window_->Present(state);
}
void WindowsPlatformBinding::SetWindowVisible(const bool visible)
{
    if (window_ && !exiting_)
        window_->SetVisible(visible);
}
void WindowsPlatformBinding::ShowError(const std::string_view message)
{
    // RunService has finished. Do not let its pending WM_QUIT immediately dismiss the fatal-error dialog.
    MSG pending{};
    while (PeekMessageW(&pending, nullptr, WM_QUIT, WM_QUIT, PM_REMOVE))
    {
    }
    const auto text = ToWide(message);
    MessageBoxW(window_ ? window_->Get() : nullptr, text.c_str(), L"Double Click Hotkey - Error", MB_OK | MB_ICONERROR);
}
void WindowsPlatformBinding::RequestExit() noexcept
{
    exiting_ = true;
    if (window_)
    {
        window_->CancelTimer();
        window_->SetTray(nullptr);
    }
    // Session-end callbacks can be followed immediately by process teardown; release native input/tray resources now.
    hook_.reset();
    if (tray_)
        tray_->Remove();
    PostQuitMessage(0);
}

PlatformResult WindowsPlatformBinding::SendF13()
{
    if (!keyboard_sender_.SendF13())
    {
        std::string message = FormatInputInjectionError("Failed to send F13", keyboard_sender_.LastErrorCode());
        const std::optional<DWORD> release_error_code = keyboard_sender_.LastReleaseErrorCode();
        if (release_error_code.has_value())
        {
            message += "; ";
            message += FormatInputInjectionError("failed to release F13 after the partial send", *release_error_code);
        }
        return {false, std::move(message)};
    }

    return {};
}

PlatformResult WindowsPlatformBinding::DoubleClick()
{
    if (!mouse_.DoubleClick())
    {
        std::string message = FormatInputInjectionError("Failed to send a double-click", mouse_.LastErrorCode());
        const std::optional<DWORD> release_error_code = mouse_.LastReleaseErrorCode();
        if (release_error_code.has_value())
        {
            message += "; ";
            message += FormatInputInjectionError("failed to release the primary mouse button after the partial send",
                                                 *release_error_code);
        }
        return {false, std::move(message)};
    }

    return {};
}

std::string WindowsPlatformBinding::FormatError(const char* const message, const unsigned long error_code)
{
    return std::string(message) + ", error code: " + std::to_string(error_code);
}

std::string WindowsPlatformBinding::FormatInputInjectionError(const char* const message, const unsigned long error_code)
{
    if (error_code != ERROR_SUCCESS)
    {
        return FormatError(message, error_code);
    }

    return std::string(message) +
           ": Windows blocked or otherwise rejected the input without reporting an error code. An integrity-level "
           "mismatch is one possible cause; if the target application is elevated, run Double Click Hotkey at the "
           "same or a higher integrity level.";
}

} // namespace double_click_hotkey::windows
