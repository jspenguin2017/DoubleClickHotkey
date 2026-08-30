#include "platform/windows/input_injector.hpp"

namespace double_click_hotkey::windows
{
InputInjector::InputInjector(const SendInputFunction send_input) noexcept : send_input_(send_input)
{
}

bool InputInjector::SendBalancedSequence(INPUT* const inputs, const UINT input_count) noexcept
{
    last_release_error_code_.reset();

    SetLastError(ERROR_SUCCESS);
    const UINT sent_input_count = send_input_(input_count, inputs, sizeof(INPUT));
    if (sent_input_count == input_count)
    {
        last_error_code_ = ERROR_SUCCESS;
        return true;
    }

    last_error_code_ = GetLastError();

    // Each sequence consists of adjacent down/up pairs. An odd prefix therefore needs one compensating release.
    if (sent_input_count < input_count && sent_input_count % 2 != 0)
    {
        SetLastError(ERROR_SUCCESS);
        if (send_input_(1, &inputs[sent_input_count], sizeof(INPUT)) != 1)
        {
            last_release_error_code_ = GetLastError();
        }
    }

    return false;
}

DWORD InputInjector::LastErrorCode() const noexcept
{
    return last_error_code_;
}

std::optional<DWORD> InputInjector::LastReleaseErrorCode() const noexcept
{
    return last_release_error_code_;
}
} // namespace double_click_hotkey::windows
