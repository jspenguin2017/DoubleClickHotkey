#pragma once

#include "double_click_hotkey/platform_binding.hpp"
#include "platform/windows/keyboard_hook.hpp"
#include "platform/windows/keyboard_sender.hpp"
#include "platform/windows/main_window.hpp"
#include "platform/windows/mouse.hpp"

#include <memory>

namespace double_click_hotkey::windows
{
class WindowsPlatformBinding final : public PlatformBinding
{
  public:
    [[nodiscard]] PlatformResult RunService(EventHandler handler) override;
    [[nodiscard]] ElapsedTime Now() override;
    [[nodiscard]] PlatformResult ScheduleTick(std::optional<ElapsedTime> deadline) override;
    void Present(const ViewState& state) override;
    void SetWindowVisible(bool visible) override;
    void ShowError(std::string_view message) override;
    void RequestExit() noexcept override;
    [[nodiscard]] PlatformResult SendF13() override;
    [[nodiscard]] PlatformResult DoubleClick() override;

  private:
    static std::string FormatError(const char* message, unsigned long error_code);
    static std::string FormatInputInjectionError(const char* message, unsigned long error_code);
    void Cleanup() noexcept;
    KeyboardSender keyboard_sender_;
    Mouse mouse_;
    std::unique_ptr<MainWindow> window_;
    std::unique_ptr<TrayIcon> tray_;
    std::unique_ptr<KeyboardHook> hook_;
    bool exiting_ = false;
};
} // namespace double_click_hotkey::windows
