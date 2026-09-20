#pragma once

#include "platform/windows/native.hpp"
#include "platform/windows/tray_icon.hpp"

#include <exception>

namespace double_click_hotkey::windows
{
class InputThreads;

class MainWindow
{
  public:
    explicit MainWindow(EventHandler handler);
    ~MainWindow();
    HWND Get() const noexcept;
    HICON SmallIcon() const noexcept;
    void SetTray(TrayIcon* tray) noexcept;
    void SetInputThreads(InputThreads* input_threads) noexcept;
    void ClearHandler() noexcept;
    void CancelTimer() noexcept;
    void SetVisible(bool visible);
    void Present(const ViewState& state);
    PlatformResult ScheduleTick(std::optional<ElapsedTime> deadline);
    bool PreTranslate(MSG& message);
    std::exception_ptr Failure() const noexcept;

  private:
    static LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) noexcept;
    LRESULT Message(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    void Emit(EventKind kind, std::string text = {});
    void Layout();
    void UpdateDpi();
    void KeepOnWorkArea();
    void UpdateLog(const ViewState& state);
    void RestoreFocus();
    UniqueFont ui_font_;
    UniqueFont log_font_;
    UniqueIcon small_icon_;
    UniqueIcon large_icon_;
    HWND log_ = nullptr;
    HWND delay_label_ = nullptr;
    HWND delay_ = nullptr;
    HWND send_ = nullptr;
    HWND last_focus_ = nullptr;
    EventHandler handler_;
    TrayIcon* tray_ = nullptr;
    InputThreads* input_threads_ = nullptr;
    std::optional<ElapsedTime> timer_deadline_;
    std::exception_ptr failure_;
    std::wstring log_text_;
    std::uint64_t log_revision_ = 0;
    std::uint64_t removed_lines_ = 0;
    bool presenting_ = false;
    // Destroy the HWND before fonts and callback state even if construction throws.
    UniqueWindow window_;
};
} // namespace double_click_hotkey::windows
