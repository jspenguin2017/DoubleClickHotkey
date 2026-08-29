#pragma once

#include <windows.h>

namespace double_click_hotkey::windows
{
enum class SingleInstanceStatus
{
    acquired,
    already_running,
    failed,
};

class SingleInstance
{
  public:
    explicit SingleInstance(const wchar_t* name) noexcept;
    ~SingleInstance();

    SingleInstance(const SingleInstance&) = delete;
    SingleInstance& operator=(const SingleInstance&) = delete;
    SingleInstance(SingleInstance&&) = delete;
    SingleInstance& operator=(SingleInstance&&) = delete;

    [[nodiscard]] SingleInstanceStatus Status() const noexcept;
    [[nodiscard]] DWORD LastErrorCode() const noexcept;

  private:
    HANDLE mutex_ = nullptr;
    SingleInstanceStatus status_ = SingleInstanceStatus::failed;
    DWORD last_error_code_ = ERROR_SUCCESS;
};
} // namespace double_click_hotkey::windows
