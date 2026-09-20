#include "platform/windows/tray_icon.hpp"

namespace double_click_hotkey::windows
{
TrayIcon::TrayIcon(HWND window, HICON icon) : menu_(CreatePopupMenu())
{
    Require(menu_.Get() != nullptr, "Create tray menu");
    Require(AppendMenuW(menu_.Get(), MF_STRING, 1, L"Show") != FALSE, "Add Show menu item");
    Require(AppendMenuW(menu_.Get(), MF_STRING, 2, L"Quit") != FALSE, "Add Quit menu item");
    taskbar_message_ = RegisterWindowMessageW(L"TaskbarCreated");
    Require(taskbar_message_ != 0, "Register TaskbarCreated");
    // Explorer may run at a lower integrity level when this app is elevated. Permit only its tray-restart message.
    Require(ChangeWindowMessageFilterEx(window, taskbar_message_, MSGFLT_ALLOW, nullptr) != FALSE,
            "Allow Explorer tray restart notification");
    data_.cbSize = sizeof(data_);
    data_.hWnd = window;
    data_.uID = 1;
    data_.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    data_.uCallbackMessage = tray_message;
    data_.hIcon = icon;
    lstrcpynW(data_.szTip, L"Double Click Hotkey", static_cast<int>(std::size(data_.szTip)));
}

TrayIcon::~TrayIcon()
{
    Remove();
}

void TrayIcon::Remove() noexcept
{
    Shell_NotifyIconW(NIM_DELETE, &data_);
    added_ = false;
}

bool TrayIcon::Add()
{
    added_ = Shell_NotifyIconW(NIM_ADD, &data_) != FALSE;
    // Version 0 notifications keep mouse coordinates in GetCursorPos and allow ordinary WM_LBUTTONUP routing.
    return added_;
}

void TrayIcon::UpdateIcon(HICON icon)
{
    data_.hIcon = icon;
    if (added_)
    {
        Shell_NotifyIconW(NIM_MODIFY, &data_);
    }
}

bool TrayIcon::Available() const
{
    return added_;
}

UINT TrayIcon::TaskbarMessage() const
{
    return taskbar_message_;
}

std::optional<EventKind> TrayIcon::Handle(const LPARAM lparam)
{
    if (lparam == WM_LBUTTONUP || lparam == NIN_SELECT || lparam == NIN_KEYSELECT)
    {
        return EventKind::show;
    }
    if (lparam == WM_RBUTTONUP || lparam == WM_CONTEXTMENU)
    {
        POINT point{};
        GetCursorPos(&point);
        SetForegroundWindow(data_.hWnd);
        const auto choice = TrackPopupMenuEx(menu_.Get(), TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON, point.x,
                                             point.y, data_.hWnd, nullptr);
        PostMessageW(data_.hWnd, WM_NULL, 0, 0);
        if (choice == 1)
        {
            return EventKind::show;
        }
        if (choice == 2)
        {
            return EventKind::quit;
        }
        // WM_NULL completes menu dismissal; cancellation also needs to return focus to the notification area.
        if (choice == 0 && added_)
            Shell_NotifyIconW(NIM_SETFOCUS, &data_);
    }
    return std::nullopt;
}
} // namespace double_click_hotkey::windows
