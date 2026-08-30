#pragma once

#include <windows.h>

#include <array>
#include <cstdint>
#include <optional>

namespace double_click_hotkey::windows
{
enum class InstanceCommand
{
    show_window,
    hide_window,
};

struct InstanceCommandChannelNames
{
    const wchar_t* event_name;
    const wchar_t* state_name;
};

inline constexpr InstanceCommandChannelNames DefaultInstanceCommandChannelNames{
    L"double-click-hotkey-command-event-v2-wzyids6rnh94128qrg5t",
    L"double-click-hotkey-command-state-v2-wzyids6rnh94128qrg5t",
};

class InstanceCommandReceiver
{
  public:
    using Handles = std::array<HANDLE, 1>;

    explicit InstanceCommandReceiver(InstanceCommandChannelNames names = DefaultInstanceCommandChannelNames) noexcept;
    ~InstanceCommandReceiver();

    InstanceCommandReceiver(const InstanceCommandReceiver&) = delete;
    InstanceCommandReceiver& operator=(const InstanceCommandReceiver&) = delete;
    InstanceCommandReceiver(InstanceCommandReceiver&&) = delete;
    InstanceCommandReceiver& operator=(InstanceCommandReceiver&&) = delete;

    [[nodiscard]] bool Initialize() noexcept;
    [[nodiscard]] const Handles& EventHandles() const noexcept;
    [[nodiscard]] std::optional<InstanceCommand> TakeLatestCommand() noexcept;
    [[nodiscard]] DWORD LastErrorCode() const noexcept;

  private:
    void Close() noexcept;

    InstanceCommandChannelNames names_;
    Handles event_handles_{};
    HANDLE state_mapping_ = nullptr;
    volatile LONG64* encoded_command_ = nullptr;
    std::uint64_t last_sequence_ = 0;
    DWORD last_error_code_ = ERROR_SUCCESS;
};

[[nodiscard]] bool SendInstanceCommand(InstanceCommand command, DWORD& error_code,
                                       InstanceCommandChannelNames names = DefaultInstanceCommandChannelNames) noexcept;
} // namespace double_click_hotkey::windows
