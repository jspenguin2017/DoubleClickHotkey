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
        input_threads_ = std::make_unique<InputThreads>(window_->Get());
        window_->SetInputThreads(input_threads_.get());
        input_threads_->Start();
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
        const auto worker_handles = input_threads_->WaitHandles();
        const std::array<HANDLE, 4> events{worker_handles[0], worker_handles[1], worker_handles[2],
                                           instance.ShowEvent()};
        while (!exiting_)
        {
            const auto wait = MsgWaitForMultipleObjectsEx(static_cast<DWORD>(events.size()), events.data(), INFINITE,
                                                          QS_ALLINPUT, MWMO_INPUTAVAILABLE);
            Require(wait != WAIT_FAILED, "Wait for application events");
            input_threads_->CheckFailure();
            if (wait == WAIT_OBJECT_0 + 3)
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
                input_threads_->CheckFailure();
            }
        }
        if (window_->Failure())
            std::rethrow_exception(window_->Failure());
        input_threads_->CheckFailure();
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
        window_->SetInputThreads(nullptr);
    }
    input_threads_.reset(); // Join both workers before destroying their notification HWND.
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
    // Cancel immediately in native callbacks; joins happen after the UI event stack has unwound.
    if (input_threads_)
        input_threads_->RequestStop();
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

} // namespace double_click_hotkey::windows
