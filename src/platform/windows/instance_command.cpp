#include "platform/windows/instance_command.hpp"

namespace double_click_hotkey::windows
{
namespace
{
constexpr std::uint64_t CommandMask = 1;
constexpr std::uint64_t MaximumSequence = (std::uint64_t{1} << 62) - 1;

// The low bit stores the command and the remaining positive LONG64 bits store its sequence. Committing both fields in
// one interlocked operation prevents a receiver from combining a new sequence with an older command.
LONG64 ReadEncodedCommand(volatile LONG64* const encoded_command) noexcept
{
    return InterlockedCompareExchange64(encoded_command, 0, 0);
}

std::uint64_t Sequence(const LONG64 encoded_command) noexcept
{
    return static_cast<std::uint64_t>(encoded_command) >> 1;
}

InstanceCommand DecodeCommand(const LONG64 encoded_command) noexcept
{
    return (static_cast<std::uint64_t>(encoded_command) & CommandMask) == 0 ? InstanceCommand::show_window
                                                                            : InstanceCommand::hide_window;
}

void StoreNextCommand(volatile LONG64* const encoded_command, const InstanceCommand command) noexcept
{
    LONG64 current = ReadEncodedCommand(encoded_command);
    while (true)
    {
        const std::uint64_t current_sequence = Sequence(current);
        const std::uint64_t next_sequence = current_sequence == MaximumSequence ? 1 : current_sequence + 1;
        const std::uint64_t command_bit = command == InstanceCommand::show_window ? 0 : CommandMask;
        const LONG64 desired = static_cast<LONG64>((next_sequence << 1) | command_bit);
        const LONG64 observed = InterlockedCompareExchange64(encoded_command, desired, current);
        if (observed == current)
        {
            return;
        }
        current = observed;
    }
}
} // namespace

InstanceCommandReceiver::InstanceCommandReceiver(const InstanceCommandChannelNames names) noexcept : names_(names)
{
}

InstanceCommandReceiver::~InstanceCommandReceiver()
{
    Close();
}

void InstanceCommandReceiver::Close() noexcept
{
    if (event_handles_[0] != nullptr)
    {
        static_cast<void>(CloseHandle(event_handles_[0]));
        event_handles_[0] = nullptr;
    }
    if (encoded_command_ != nullptr)
    {
        static_cast<void>(UnmapViewOfFile(const_cast<const LONG64*>(encoded_command_)));
        encoded_command_ = nullptr;
    }
    if (state_mapping_ != nullptr)
    {
        static_cast<void>(CloseHandle(state_mapping_));
        state_mapping_ = nullptr;
    }
    last_sequence_ = 0;
}

bool InstanceCommandReceiver::Initialize() noexcept
{
    if (event_handles_[0] != nullptr || state_mapping_ != nullptr || encoded_command_ != nullptr)
    {
        last_error_code_ = ERROR_ALREADY_EXISTS;
        return false;
    }

    state_mapping_ = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0,
                                        static_cast<DWORD>(sizeof(*encoded_command_)), names_.state_name);
    if (state_mapping_ == nullptr)
    {
        last_error_code_ = GetLastError();
        return false;
    }

    encoded_command_ =
        static_cast<volatile LONG64*>(MapViewOfFile(state_mapping_, FILE_MAP_WRITE, 0, 0, sizeof(*encoded_command_)));
    if (encoded_command_ == nullptr)
    {
        last_error_code_ = GetLastError();
        Close();
        return false;
    }

    static_cast<void>(InterlockedExchange64(encoded_command_, 0));
    event_handles_[0] = CreateEventW(nullptr, FALSE, FALSE, names_.event_name);
    if (event_handles_[0] == nullptr)
    {
        last_error_code_ = GetLastError();
        Close();
        return false;
    }

    last_sequence_ = 0;
    last_error_code_ = ERROR_SUCCESS;
    return true;
}

const InstanceCommandReceiver::Handles& InstanceCommandReceiver::EventHandles() const noexcept
{
    return event_handles_;
}

std::optional<InstanceCommand> InstanceCommandReceiver::TakeLatestCommand() noexcept
{
    if (encoded_command_ == nullptr)
    {
        return std::nullopt;
    }

    const LONG64 encoded_command = ReadEncodedCommand(encoded_command_);
    const std::uint64_t sequence = Sequence(encoded_command);
    if (sequence == 0 || sequence == last_sequence_)
    {
        return std::nullopt;
    }

    last_sequence_ = sequence;
    return DecodeCommand(encoded_command);
}

DWORD InstanceCommandReceiver::LastErrorCode() const noexcept
{
    return last_error_code_;
}

bool SendInstanceCommand(const InstanceCommand command, DWORD& error_code,
                         const InstanceCommandChannelNames names) noexcept
{
    const HANDLE event_handle = OpenEventW(EVENT_MODIFY_STATE, FALSE, names.event_name);
    if (event_handle == nullptr)
    {
        error_code = GetLastError();
        return false;
    }

    const HANDLE state_mapping = OpenFileMappingW(FILE_MAP_WRITE, FALSE, names.state_name);
    if (state_mapping == nullptr)
    {
        error_code = GetLastError();
        static_cast<void>(CloseHandle(event_handle));
        return false;
    }

    auto* const encoded_command =
        static_cast<volatile LONG64*>(MapViewOfFile(state_mapping, FILE_MAP_WRITE, 0, 0, sizeof(LONG64)));
    if (encoded_command == nullptr)
    {
        error_code = GetLastError();
        static_cast<void>(CloseHandle(state_mapping));
        static_cast<void>(CloseHandle(event_handle));
        return false;
    }

    StoreNextCommand(encoded_command, command);
    const bool succeeded = SetEvent(event_handle) != 0;
    error_code = succeeded ? ERROR_SUCCESS : GetLastError();
    static_cast<void>(UnmapViewOfFile(const_cast<const LONG64*>(encoded_command)));
    static_cast<void>(CloseHandle(state_mapping));
    static_cast<void>(CloseHandle(event_handle));
    return succeeded;
}
} // namespace double_click_hotkey::windows
