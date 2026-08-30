#pragma once

#include <string_view>

namespace double_click_hotkey::windows
{
class Console
{
  public:
    void Hide() noexcept;
    void Show() noexcept;
    void WriteLine(std::string_view message);
    void ReportError(std::string_view message, bool wait_for_key = true);

  private:
    static void WaitForKey();
};
} // namespace double_click_hotkey::windows
