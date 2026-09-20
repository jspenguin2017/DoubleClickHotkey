#pragma once

#include "platform/windows/native.hpp"

#include <shellapi.h>

namespace double_click_hotkey::windows
{
constexpr UINT tray_message = WM_APP + 1;

class TrayIcon
{
  public:
    TrayIcon(HWND window, HICON icon);
    ~TrayIcon();
    void Remove() noexcept;
    bool Add();
    void UpdateIcon(HICON icon);
    bool Available() const;
    UINT TaskbarMessage() const;
    std::optional<EventKind> Handle(LPARAM lparam);

  private:
    NOTIFYICONDATAW data_{};
    UniqueMenu menu_;
    UINT taskbar_message_ = 0;
    bool added_ = false;
};
} // namespace double_click_hotkey::windows
