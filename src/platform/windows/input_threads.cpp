#include "platform/windows/input_threads.hpp"

#include "platform/windows/keyboard_hook.hpp"
#include "platform/windows/mouse.hpp"

#include <cerrno>
#include <process.h>

namespace double_click_hotkey::windows
{
namespace
{
constexpr UINT DoubleClickMessage = WM_APP + 1; // Input thread queue only; never dispatched by a window.
constexpr WPARAM HasReleaseError = WPARAM{1} << 32;
static_assert(std::atomic<std::uint64_t>::is_always_lock_free);
static_assert(std::atomic<bool>::is_always_lock_free);

HANDLE CreateManualEvent()
{
    const HANDLE event = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    Require(event != nullptr, "Create input thread event");
    return event;
}

HANDLE StartThread(unsigned int(__stdcall* entry)(void*), void* context, unsigned int* id)
{
    const auto thread = _beginthreadex(nullptr, 0, entry, context, 0, id);
    if (!thread)
        throw std::runtime_error("Start input service thread, C runtime error code: " + std::to_string(errno));
    return reinterpret_cast<HANDLE>(thread);
}
} // namespace

InputThreads::InputThreads(HWND receiver)
    : receiver_(receiver), input_ready_(CreateManualEvent()), hook_ready_(CreateManualEvent()),
      input_stop_(CreateManualEvent()), hook_stop_(CreateManualEvent()), failure_event_(CreateManualEvent())
{
}

InputThreads::~InputThreads()
{
    Stop();
}

void InputThreads::Start()
{
    input_thread_.Reset(StartThread(InputEntry, this, &input_id_));
    AwaitReady(input_ready_.Get(), input_thread_.Get());
    hook_thread_.Reset(StartThread(HookEntry, this, nullptr));
    AwaitReady(hook_ready_.Get(), hook_thread_.Get());
}

void InputThreads::AwaitReady(HANDLE ready, HANDLE thread) const
{
    const std::array<HANDLE, 3> handles{failure_event_.Get(), thread, ready};
    const auto wait = WaitForMultipleObjects(static_cast<DWORD>(handles.size()), handles.data(), FALSE, INFINITE);
    Require(wait != WAIT_FAILED, "Wait for input thread readiness");
    CheckFailure();
    if (wait != WAIT_OBJECT_0 + 2)
        throw std::runtime_error("Input service thread exited before becoming ready.");
}

bool InputThreads::SignalReady(HANDLE ready) noexcept
{
    if (SetEvent(ready))
        return true;
    ReportFailure(Failure::signal_ready, GetLastError());
    return false;
}

bool InputThreads::Stopping() const noexcept
{
    return stopping_.load(std::memory_order_acquire);
}

void InputThreads::RequestStop() noexcept
{
    stopping_.store(true, std::memory_order_release);
    static_cast<void>(SetEvent(input_stop_.Get()));
}

void InputThreads::Stop() noexcept
{
    RequestStop();
    // Keep the hook pumping until the last input sequence (including a compensating release) finishes.
    if (input_thread_.Get())
        static_cast<void>(WaitForSingleObject(input_thread_.Get(), INFINITE));
    static_cast<void>(SetEvent(hook_stop_.Get()));
    if (hook_thread_.Get())
        static_cast<void>(WaitForSingleObject(hook_thread_.Get(), INFINITE));
    hook_thread_.Reset();
    input_thread_.Reset();
}

std::array<HANDLE, 3> InputThreads::WaitHandles() const noexcept
{
    return {failure_event_.Get(), input_thread_.Get(), hook_thread_.Get()};
}

void InputThreads::ReportFailure(const Failure operation, const DWORD error) noexcept
{
    const auto value = (static_cast<std::uint64_t>(operation) << 32) | error;
    std::uint64_t expected = 0;
    static_cast<void>(
        failure_.compare_exchange_strong(expected, value, std::memory_order_release, std::memory_order_relaxed));
    RequestStop();
    // The event remains observable even if the UI queue is full. The HWND message also reaches native modal loops.
    static_cast<void>(SetEvent(failure_event_.Get()));
    static_cast<void>(PostMessageW(receiver_, WorkerFailureMessage, 0, 0));
}

void InputThreads::CheckFailure() const
{
    const auto failure = failure_.load(std::memory_order_acquire);
    if (failure)
    {
        const char* operation = "Input service thread failed";
        switch (static_cast<Failure>(failure >> 32))
        {
        case Failure::install_hook:
            operation = "Install F13 keyboard hook";
            break;
        case Failure::queue_click:
            operation = "Queue double-click";
            break;
        case Failure::report_click:
            operation = "Report double-click failure";
            break;
        case Failure::wait_input:
            operation = "Wait for double-click requests";
            break;
        case Failure::wait_hook:
            operation = "Wait for keyboard hook events";
            break;
        case Failure::signal_ready:
            operation = "Signal input thread readiness";
            break;
        case Failure::input_exit:
            operation = "Double-click thread exited unexpectedly";
            break;
        case Failure::hook_exit:
            operation = "Keyboard hook thread exited unexpectedly";
            break;
        }
        throw std::runtime_error(NativeError(operation, static_cast<DWORD>(failure)));
    }
    if (!Stopping())
    {
        for (const HANDLE thread : {input_thread_.Get(), hook_thread_.Get()})
        {
            if (thread && WaitForSingleObject(thread, 0) == WAIT_OBJECT_0)
                throw std::runtime_error("Input service thread exited unexpectedly.");
        }
    }
}

void InputThreads::QueueDoubleClick() noexcept
{
    if (!Stopping() && !PostThreadMessageW(input_id_, DoubleClickMessage, 0, 0))
        ReportFailure(Failure::queue_click, GetLastError());
}

unsigned int __stdcall InputThreads::InputEntry(void* const context) noexcept
{
    auto& self = *static_cast<InputThreads*>(context);
    try
    {
        self.RunInput();
    }
    catch (...)
    {
        self.ReportFailure(Failure::input_exit, ERROR_UNHANDLED_EXCEPTION);
    }
    return 0;
}

unsigned int __stdcall InputThreads::HookEntry(void* const context) noexcept
{
    auto& self = *static_cast<InputThreads*>(context);
    try
    {
        self.RunHook();
    }
    catch (...)
    {
        self.ReportFailure(Failure::hook_exit, ERROR_UNHANDLED_EXCEPTION);
    }
    return 0;
}

void InputThreads::RunInput()
{
    Mouse mouse; // Injector/error state is owned exclusively by this thread.
    MSG message{};
    static_cast<void>(PeekMessageW(&message, nullptr, WM_USER, WM_USER, PM_NOREMOVE));
    if (!SignalReady(input_ready_.Get()))
        return;
    const HANDLE stop = input_stop_.Get();
    while (!Stopping())
    {
        const auto wait = MsgWaitForMultipleObjectsEx(1, &stop, INFINITE, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
        if (wait == WAIT_OBJECT_0)
            return;
        if (wait == WAIT_FAILED)
        {
            ReportFailure(Failure::wait_input, GetLastError());
            return;
        }
        for (int count = 0; count < 128 && !Stopping() && PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE); ++count)
        {
            if (message.message == WM_QUIT)
            {
                ReportFailure(Failure::input_exit, ERROR_SUCCESS);
                return;
            }
            if (!message.hwnd && message.message == DoubleClickMessage && !Stopping() && !mouse.DoubleClick())
            {
                const auto release_error = mouse.LastReleaseErrorCode();
                const WPARAM error = mouse.LastErrorCode() | (release_error ? HasReleaseError : 0);
                if (!PostMessageW(receiver_, DoubleClickFailureMessage, error, release_error.value_or(ERROR_SUCCESS)))
                {
                    ReportFailure(Failure::report_click, GetLastError());
                    return;
                }
            }
        }
    }
}

void InputThreads::RunHook()
{
    KeyboardHook hook; // Install and unhook on the thread that delivers the callback.
    if (!hook.Install(*this))
    {
        ReportFailure(Failure::install_hook, hook.LastErrorCode());
        return;
    }
    if (!SignalReady(hook_ready_.Get()))
        return;
    const HANDLE stop = hook_stop_.Get();
    while (true)
    {
        const auto wait = MsgWaitForMultipleObjectsEx(1, &stop, INFINITE, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
        if (wait == WAIT_OBJECT_0)
            return;
        if (wait == WAIT_FAILED)
        {
            ReportFailure(Failure::wait_hook, GetLastError());
            return;
        }
        MSG message{};
        for (int count = 0; count < 128 && PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE); ++count)
        {
            if (WaitForSingleObject(stop, 0) == WAIT_OBJECT_0)
                return;
            if (message.message == WM_QUIT)
            {
                ReportFailure(Failure::hook_exit, ERROR_SUCCESS);
                return;
            }
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }
}

std::string InputThreads::DoubleClickFailureText(const WPARAM error, const LPARAM release_error)
{
    std::string message = FormatInputInjectionError("Failed to send a double-click", static_cast<DWORD>(error));
    if (error & HasReleaseError)
    {
        message += "; ";
        message += FormatInputInjectionError("failed to release the primary mouse button after the partial send",
                                             static_cast<DWORD>(release_error));
    }
    return message;
}
} // namespace double_click_hotkey::windows
