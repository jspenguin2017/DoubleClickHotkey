#pragma once

#include <string_view>

namespace double_click_hotkey::windows
{
class Console
{
  public:
    [[nodiscard]] bool IsVisible() const noexcept;
    void Hide() noexcept;
    void Show() noexcept;
    void ToggleVisibility() noexcept;
    void ReportError(std::string_view message);

  private:
    static void WaitForKey();
};
} // namespace double_click_hotkey::windows
