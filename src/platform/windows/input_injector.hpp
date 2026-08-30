#pragma once

#include <windows.h>

#include <optional>

namespace double_click_hotkey::windows
{
class InputInjector
{
  public:
    using SendInputFunction = decltype(&::SendInput);

    explicit InputInjector(SendInputFunction send_input = &::SendInput) noexcept;

    // The sequence must contain adjacent down/up pairs.
    [[nodiscard]] bool SendBalancedSequence(INPUT* inputs, UINT input_count) noexcept;
    [[nodiscard]] DWORD LastErrorCode() const noexcept;
    [[nodiscard]] std::optional<DWORD> LastReleaseErrorCode() const noexcept;

  private:
    SendInputFunction send_input_;
    DWORD last_error_code_ = ERROR_SUCCESS;
    std::optional<DWORD> last_release_error_code_;
};
} // namespace double_click_hotkey::windows
