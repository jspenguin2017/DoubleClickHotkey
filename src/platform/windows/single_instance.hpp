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

enum class SingleInstanceProbeStatus
{
    running,
    not_running,
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
    [[nodiscard]] const char* LastErrorMessage() const noexcept;
    [[nodiscard]] DWORD LastErrorCode() const noexcept;

  private:
    HANDLE mutex_ = nullptr;
    SingleInstanceStatus status_ = SingleInstanceStatus::failed;
    const char* last_error_message_ = "Failed to reserve the session-local single-instance mutex";
    DWORD last_error_code_ = ERROR_SUCCESS;
};

[[nodiscard]] SingleInstanceProbeStatus ProbeSingleInstance(const wchar_t* name, DWORD& error_code) noexcept;
} // namespace double_click_hotkey::windows
