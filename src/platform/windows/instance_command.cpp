#include "platform/windows/instance_command.hpp"

#include "platform/windows/named_object_security.hpp"

namespace double_click_hotkey::windows
{
namespace
{
constexpr std::uint64_t CommandMask = 1;
constexpr std::uint64_t MaximumSequence = (std::uint64_t{1} << 62) - 1;

// Visibility commands are intentionally non-sensitive. These descriptors let a normal interactive process write to an
// elevated receiver in the same session, while granting only the mapping and event rights used by the protocol.
constexpr wchar_t StateSecurityDescriptor[] = L"D:P(A;;0x00000002;;;IU)S:(ML;;NW;;;ME)";
constexpr wchar_t EventSecurityDescriptor[] = L"D:P(A;;0x00100002;;;IU)S:(ML;;NW;;;ME)";

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
        last_error_message_ = "Failed to initialize an instance command receiver that is already initialized";
        last_error_code_ = ERROR_ALREADY_EXISTS;
        return false;
    }

    NamedObjectSecurityAttributes state_security_attributes(StateSecurityDescriptor);
    if (!state_security_attributes.IsValid())
    {
        last_error_message_ = "Failed to prepare security for the instance command state mapping";
        last_error_code_ = state_security_attributes.LastErrorCode();
        return false;
    }

    SetLastError(ERROR_SUCCESS);
    state_mapping_ = CreateFileMappingW(INVALID_HANDLE_VALUE, state_security_attributes.Get(), PAGE_READWRITE, 0,
                                        static_cast<DWORD>(sizeof(*encoded_command_)), names_.state_name);
    if (state_mapping_ == nullptr)
    {
        last_error_message_ = "Failed to create the instance command state mapping";
        last_error_code_ = GetLastError();
        return false;
    }
    const DWORD state_creation_error_code = GetLastError();
    if (state_creation_error_code == ERROR_ALREADY_EXISTS)
    {
        last_error_message_ =
            "Cannot start the service because the named instance command state mapping already exists";
        last_error_code_ = ERROR_ALREADY_EXISTS;
        Close();
        return false;
    }

    encoded_command_ =
        static_cast<volatile LONG64*>(MapViewOfFile(state_mapping_, FILE_MAP_WRITE, 0, 0, sizeof(*encoded_command_)));
    if (encoded_command_ == nullptr)
    {
        last_error_message_ = "Failed to map the instance command state into the service process";
        last_error_code_ = GetLastError();
        Close();
        return false;
    }

    static_cast<void>(InterlockedExchange64(encoded_command_, 0));

    NamedObjectSecurityAttributes event_security_attributes(EventSecurityDescriptor);
    if (!event_security_attributes.IsValid())
    {
        last_error_message_ = "Failed to prepare security for the instance command event";
        last_error_code_ = event_security_attributes.LastErrorCode();
        Close();
        return false;
    }

    SetLastError(ERROR_SUCCESS);
    event_handles_[0] = CreateEventExW(event_security_attributes.Get(), names_.event_name, 0, SYNCHRONIZE);
    if (event_handles_[0] == nullptr)
    {
        last_error_message_ = "Failed to create the instance command event";
        last_error_code_ = GetLastError();
        Close();
        return false;
    }
    const DWORD event_creation_error_code = GetLastError();
    if (event_creation_error_code == ERROR_ALREADY_EXISTS)
    {
        last_error_message_ = "Cannot start the service because the named instance command event already exists";
        last_error_code_ = ERROR_ALREADY_EXISTS;
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

const char* InstanceCommandReceiver::LastErrorMessage() const noexcept
{
    return last_error_message_;
}

DWORD InstanceCommandReceiver::LastErrorCode() const noexcept
{
    return last_error_code_;
}

bool SendInstanceCommand(const InstanceCommand command, InstanceCommandError& error,
                         const InstanceCommandChannelNames names) noexcept
{
    const HANDLE event_handle = OpenEventW(EVENT_MODIFY_STATE, FALSE, names.event_name);
    if (event_handle == nullptr)
    {
        error.message = "Failed to open the running instance's command event";
        error.code = GetLastError();
        return false;
    }

    const HANDLE state_mapping = OpenFileMappingW(FILE_MAP_WRITE, FALSE, names.state_name);
    if (state_mapping == nullptr)
    {
        error.message = "Failed to open the running instance's command state mapping";
        error.code = GetLastError();
        static_cast<void>(CloseHandle(event_handle));
        return false;
    }

    auto* const encoded_command =
        static_cast<volatile LONG64*>(MapViewOfFile(state_mapping, FILE_MAP_WRITE, 0, 0, sizeof(LONG64)));
    if (encoded_command == nullptr)
    {
        error.message = "Failed to map the running instance's command state into the sender process";
        error.code = GetLastError();
        static_cast<void>(CloseHandle(state_mapping));
        static_cast<void>(CloseHandle(event_handle));
        return false;
    }

    StoreNextCommand(encoded_command, command);
    const bool succeeded = SetEvent(event_handle) != 0;
    if (succeeded)
    {
        error = {};
    }
    else
    {
        error.message = "Failed to signal the running instance's command event";
        error.code = GetLastError();
    }
    static_cast<void>(UnmapViewOfFile(const_cast<const LONG64*>(encoded_command)));
    static_cast<void>(CloseHandle(state_mapping));
    static_cast<void>(CloseHandle(event_handle));
    return succeeded;
}
} // namespace double_click_hotkey::windows
