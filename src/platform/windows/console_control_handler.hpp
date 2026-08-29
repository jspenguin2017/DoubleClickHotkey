#pragma once

#include <windows.h>

#include <atomic>

namespace double_click_hotkey::windows
{
class ConsoleControlHandler
{
  public:
    ConsoleControlHandler() = default;
    ~ConsoleControlHandler();

    ConsoleControlHandler(const ConsoleControlHandler&) = delete;
    ConsoleControlHandler& operator=(const ConsoleControlHandler&) = delete;
    ConsoleControlHandler(ConsoleControlHandler&&) = delete;
    ConsoleControlHandler& operator=(ConsoleControlHandler&&) = delete;

    [[nodiscard]] bool Install() noexcept;
    [[nodiscard]] DWORD LastErrorCode() const noexcept;

  private:
    static BOOL WINAPI HandleControlEvent(DWORD event_type) noexcept;

    static std::atomic<DWORD> main_thread_id_;
    bool installed_ = false;
    DWORD last_error_code_ = ERROR_SUCCESS;
};
} // namespace double_click_hotkey::windows
