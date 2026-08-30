#include "platform/windows/single_instance.hpp"

#include "platform/windows/named_object_security.hpp"

namespace double_click_hotkey::windows
{
namespace
{
// The mutex is only an existence marker. Interactive callers need SYNCHRONIZE to recognize an elevated reservation,
// and the explicit medium label keeps its integrity policy independent of the creator's elevation.
constexpr wchar_t MutexSecurityDescriptor[] = L"D:P(A;;0x00100000;;;IU)S:(ML;;NW;;;ME)";
} // namespace

SingleInstance::SingleInstance(const wchar_t* const name) noexcept
{
    NamedObjectSecurityAttributes security_attributes(MutexSecurityDescriptor);
    if (!security_attributes.IsValid())
    {
        last_error_message_ = "Failed to prepare security for the session-local single-instance mutex";
        last_error_code_ = security_attributes.LastErrorCode();
        return;
    }

    SetLastError(ERROR_SUCCESS);
    mutex_ = CreateMutexExW(security_attributes.Get(), name, 0, SYNCHRONIZE);
    if (mutex_ == nullptr)
    {
        last_error_message_ = "Failed to create or open the session-local single-instance mutex";
        last_error_code_ = GetLastError();
        return;
    }

    const DWORD creation_error_code = GetLastError();
    status_ = creation_error_code == ERROR_ALREADY_EXISTS ? SingleInstanceStatus::already_running
                                                          : SingleInstanceStatus::acquired;
    last_error_code_ = ERROR_SUCCESS;
}

SingleInstance::~SingleInstance()
{
    if (mutex_ == nullptr)
    {
        return;
    }

    static_cast<void>(CloseHandle(mutex_));
}

SingleInstanceStatus SingleInstance::Status() const noexcept
{
    return status_;
}

const char* SingleInstance::LastErrorMessage() const noexcept
{
    return last_error_message_;
}

DWORD SingleInstance::LastErrorCode() const noexcept
{
    return last_error_code_;
}

SingleInstanceProbeStatus ProbeSingleInstance(const wchar_t* const name, DWORD& error_code) noexcept
{
    const HANDLE mutex = OpenMutexW(SYNCHRONIZE, FALSE, name);
    if (mutex == nullptr)
    {
        error_code = GetLastError();
        return error_code == ERROR_FILE_NOT_FOUND ? SingleInstanceProbeStatus::not_running
                                                  : SingleInstanceProbeStatus::failed;
    }

    static_cast<void>(CloseHandle(mutex));
    error_code = ERROR_SUCCESS;
    return SingleInstanceProbeStatus::running;
}
} // namespace double_click_hotkey::windows
