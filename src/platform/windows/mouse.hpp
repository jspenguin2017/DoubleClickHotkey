#pragma once

#include <windows.h>

namespace double_click_hotkey::windows
{
class Mouse
{
  public:
    [[nodiscard]] bool DoubleClick() noexcept;
    [[nodiscard]] DWORD LastErrorCode() const noexcept;

  private:
    DWORD last_error_code_ = ERROR_SUCCESS;
};
} // namespace double_click_hotkey::windows
