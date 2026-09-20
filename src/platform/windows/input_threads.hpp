#pragma once

#include "platform/windows/native.hpp"

#include <array>
#include <atomic>
#include <cstdint>

namespace double_click_hotkey::windows
{
class InputThreads
{
  public:
    explicit InputThreads(HWND receiver);
    ~InputThreads();
    InputThreads(const InputThreads&) = delete;
    InputThreads& operator=(const InputThreads&) = delete;

    static constexpr UINT DoubleClickFailureMessage = WM_APP + 2;
    static constexpr UINT WorkerFailureMessage = WM_APP + 3;

    // Start/CheckFailure/Stop and destruction belong to the UI thread. QueueDoubleClick belongs to the hook thread.
    void Start();
    void RequestStop() noexcept;
    void Stop() noexcept;
    void QueueDoubleClick() noexcept;
    void CheckFailure() const;
    [[nodiscard]] std::array<HANDLE, 3> WaitHandles() const noexcept;
    [[nodiscard]] static std::string DoubleClickFailureText(WPARAM error, LPARAM release_error);

  private:
    enum class Failure : std::uint32_t
    {
        install_hook = 1,
        queue_click,
        report_click,
        wait_input,
        wait_hook,
        signal_ready,
        input_exit,
        hook_exit
    };

    static unsigned int __stdcall InputEntry(void* context) noexcept;
    static unsigned int __stdcall HookEntry(void* context) noexcept;
    void RunInput();
    void RunHook();
    void ReportFailure(Failure operation, DWORD error) noexcept;
    void AwaitReady(HANDLE ready, HANDLE thread) const;
    [[nodiscard]] bool SignalReady(HANDLE ready) noexcept;
    [[nodiscard]] bool Stopping() const noexcept;

    const HWND receiver_;
    UniqueHandle input_ready_;
    UniqueHandle hook_ready_;
    UniqueHandle input_stop_;
    UniqueHandle hook_stop_;
    UniqueHandle failure_event_;
    UniqueHandle input_thread_;
    UniqueHandle hook_thread_;
    unsigned int input_id_ = 0;
    std::atomic<bool> stopping_{false};
    // Publish the operation and error together, including a valid error code of zero. First failure wins.
    std::atomic<std::uint64_t> failure_{0};
};
} // namespace double_click_hotkey::windows
