#include "double_click_hotkey/platform_factory.hpp"

#include "platform/windows/windows_platform_binding.hpp"

#include <memory>

namespace double_click_hotkey
{
std::unique_ptr<PlatformBinding> CreatePlatformBinding(const bool hide_window_immediately)
{
    return std::make_unique<windows::WindowsPlatformBinding>(hide_window_immediately);
}
} // namespace double_click_hotkey
