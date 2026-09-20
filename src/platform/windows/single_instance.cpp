#include "platform/windows/single_instance.hpp"

#include "platform/windows/named_object_security.hpp"

namespace double_click_hotkey::windows
{
namespace
{
// Keep the existing session-local ownership name so an older console service cannot also install a hook.
constexpr wchar_t MutexName[] = L"Local\\double-click-hotkey-mutex-v3-dd74d3c1-ded5-4d6c-869c-f06eb80200ee";
constexpr wchar_t ShowEventName[] = L"Local\\double-click-hotkey-show-v4-dd74d3c1-ded5-4d6c-869c-f06eb80200ee";
// Show is non-sensitive. Grant only the rights needed to recognize ownership and signal Show across elevations.
constexpr wchar_t MutexSecurity[] = L"D:P(A;;0x00100000;;;IU)S:(ML;;NW;;;ME)";
constexpr wchar_t EventSecurity[] = L"D:P(A;;0x00100002;;;IU)S:(ML;;NW;;;ME)";
} // namespace

bool SingleInstance::AcquireOrShow()
{
    NamedObjectSecurityAttributes security(MutexSecurity);
    if (!security.IsValid())
        throw std::runtime_error(NativeError("Prepare instance security", security.LastErrorCode()));
    const auto deadline = GetTickCount64() + 2000;
    while (true)
    {
        SetLastError(ERROR_SUCCESS);
        mutex_.Reset(CreateMutexExW(security.Get(), MutexName, 0, SYNCHRONIZE));
        const auto error = GetLastError();
        if (!mutex_.Get())
            throw std::runtime_error(NativeError("Reserve application instance", error));
        if (error != ERROR_ALREADY_EXISTS)
            return true;
        UniqueHandle receiver(OpenEventW(EVENT_MODIFY_STATE, FALSE, ShowEventName));
        if (receiver.Get())
        {
            const auto window = FindWindowW(MainWindowClass, nullptr);
            DWORD pid = 0;
            if (window && GetWindowThreadProcessId(window, &pid))
                AllowSetForegroundWindow(pid);
            Require(SetEvent(receiver.Get()) != FALSE, "Show existing Double Click Hotkey window");
            mutex_.Reset();
            return false;
        }
        const auto event_error = GetLastError();
        // Do not keep an exiting owner's existence marker alive while waiting for a replacement owner.
        mutex_.Reset();
        if (event_error != ERROR_FILE_NOT_FOUND)
            throw std::runtime_error(NativeError("Open instance Show event", event_error));
        if (GetTickCount64() >= deadline)
            throw std::runtime_error("Double Click Hotkey did not become ready within two seconds. Try again; close "
                                     "any older console version first.");
        Sleep(40);
    }
}

void SingleInstance::StartListening()
{
    NamedObjectSecurityAttributes security(EventSecurity);
    if (!security.IsValid())
        throw std::runtime_error(NativeError("Prepare Show event security", security.LastErrorCode()));
    SetLastError(ERROR_SUCCESS);
    event_.Reset(CreateEventExW(security.Get(), ShowEventName, 0, SYNCHRONIZE));
    const auto error = GetLastError();
    if (!event_.Get() || error == ERROR_ALREADY_EXISTS)
        throw std::runtime_error(NativeError("Create instance Show event", error));
}
HANDLE SingleInstance::ShowEvent() const noexcept
{
    return event_.Get();
}
} // namespace double_click_hotkey::windows
