#include "platform/windows/single_instance.hpp"

namespace double_click_hotkey::windows
{
SingleInstance::SingleInstance(const wchar_t* const name) noexcept
{
    SetLastError(ERROR_SUCCESS);
    mutex_ = CreateMutexW(nullptr, TRUE, name);
    if (mutex_ == nullptr)
    {
        last_error_code_ = GetLastError();
        return;
    }

    last_error_code_ = GetLastError();
    status_ = last_error_code_ == ERROR_ALREADY_EXISTS ? SingleInstanceStatus::already_running
                                                       : SingleInstanceStatus::acquired;
}

SingleInstance::~SingleInstance()
{
    if (mutex_ == nullptr)
    {
        return;
    }

    if (status_ == SingleInstanceStatus::acquired)
    {
        static_cast<void>(ReleaseMutex(mutex_));
    }
    static_cast<void>(CloseHandle(mutex_));
}

SingleInstanceStatus SingleInstance::Status() const noexcept
{
    return status_;
}

DWORD SingleInstance::LastErrorCode() const noexcept
{
    return last_error_code_;
}
} // namespace double_click_hotkey::windows
