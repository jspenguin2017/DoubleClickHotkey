#pragma once

#include <windows.h>

namespace double_click_hotkey::windows
{
// Shared by the sender and hook. The process-local address identifies our own input; it is not an IPC capability.
inline const unsigned char F13InputTagStorage = 0;
inline ULONG_PTR F13InputTag() noexcept
{
    return reinterpret_cast<ULONG_PTR>(&F13InputTagStorage);
}
} // namespace double_click_hotkey::windows
