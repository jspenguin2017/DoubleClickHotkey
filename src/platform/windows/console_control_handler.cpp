#include "platform/windows/console_control_handler.hpp"

namespace double_click_hotkey::windows
{
std::atomic<DWORD> ConsoleControlHandler::main_thread_id_ = 0;

ConsoleControlHandler::~ConsoleControlHandler()
{
    if (installed_)
    {
        static_cast<void>(SetConsoleCtrlHandler(&HandleControlEvent, FALSE));
        main_thread_id_.store(0, std::memory_order_release);
    }
}

bool ConsoleControlHandler::Install() noexcept
{
    MSG message{};
    static_cast<void>(PeekMessageW(&message, nullptr, WM_USER, WM_USER, PM_NOREMOVE));

    DWORD expected_thread_id = 0;
    if (!main_thread_id_.compare_exchange_strong(expected_thread_id, GetCurrentThreadId(), std::memory_order_acq_rel))
    {
        last_error_code_ = ERROR_ALREADY_EXISTS;
        return false;
    }

    if (!SetConsoleCtrlHandler(&HandleControlEvent, TRUE))
    {
        last_error_code_ = GetLastError();
        main_thread_id_.store(0, std::memory_order_release);
        return false;
    }

    installed_ = true;
    last_error_code_ = ERROR_SUCCESS;
    return true;
}

DWORD ConsoleControlHandler::LastErrorCode() const noexcept
{
    return last_error_code_;
}

BOOL WINAPI ConsoleControlHandler::HandleControlEvent(const DWORD) noexcept
{
    const DWORD main_thread_id = main_thread_id_.load(std::memory_order_acquire);
    if (main_thread_id == 0)
    {
        return FALSE;
    }

    return PostThreadMessageW(main_thread_id, WM_QUIT, 0, 0) != 0 ? TRUE : FALSE;
}
} // namespace double_click_hotkey::windows
