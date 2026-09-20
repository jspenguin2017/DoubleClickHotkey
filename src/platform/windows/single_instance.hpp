#pragma once

#include "platform/windows/native.hpp"

namespace double_click_hotkey::windows
{
inline constexpr wchar_t MainWindowClass[] = L"DoubleClickHotkey.MainWindow";
class SingleInstance
{
  public:
    // False means Show was delivered to the existing owner. Failed/bounded-out activation throws.
    bool AcquireOrShow();
    void StartListening();
    HANDLE ShowEvent() const noexcept;

  private:
    UniqueHandle mutex_;
    UniqueHandle event_;
};
} // namespace double_click_hotkey::windows
