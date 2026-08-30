#include "double_click_hotkey/platform_factory.hpp"

#include "platform/windows/windows_platform_binding.hpp"

#include <memory>

namespace double_click_hotkey
{
std::unique_ptr<PlatformBinding> CreatePlatformBinding()
{
    return std::make_unique<windows::WindowsPlatformBinding>();
}
} // namespace double_click_hotkey
