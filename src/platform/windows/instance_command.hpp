#pragma once

#include <windows.h>

#include <array>
#include <cstddef>

namespace double_click_hotkey::windows
{
enum class InstanceCommand
{
    show_window,
    hide_window,
};

class InstanceCommandReceiver
{
  public:
    using Handles = std::array<HANDLE, 2>;

    InstanceCommandReceiver() = default;
    ~InstanceCommandReceiver();

    InstanceCommandReceiver(const InstanceCommandReceiver&) = delete;
    InstanceCommandReceiver& operator=(const InstanceCommandReceiver&) = delete;
    InstanceCommandReceiver(InstanceCommandReceiver&&) = delete;
    InstanceCommandReceiver& operator=(InstanceCommandReceiver&&) = delete;

    [[nodiscard]] bool Initialize() noexcept;
    [[nodiscard]] const Handles& EventHandles() const noexcept;
    [[nodiscard]] InstanceCommand CommandAt(std::size_t index) const noexcept;
    [[nodiscard]] DWORD LastErrorCode() const noexcept;

  private:
    Handles event_handles_{};
    DWORD last_error_code_ = ERROR_SUCCESS;
};

[[nodiscard]] bool SendInstanceCommand(InstanceCommand command, DWORD& error_code) noexcept;
} // namespace double_click_hotkey::windows
