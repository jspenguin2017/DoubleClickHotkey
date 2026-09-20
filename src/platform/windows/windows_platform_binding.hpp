#pragma once

#include "double_click_hotkey/platform_binding.hpp"
#include "platform/windows/input_threads.hpp"
#include "platform/windows/keyboard_sender.hpp"
#include "platform/windows/main_window.hpp"

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

  private:
    void Cleanup() noexcept;
    KeyboardSender keyboard_sender_;
    std::unique_ptr<MainWindow> window_;
    std::unique_ptr<TrayIcon> tray_;
    std::unique_ptr<InputThreads> input_threads_;
    bool exiting_ = false;
};
} // namespace double_click_hotkey::windows
