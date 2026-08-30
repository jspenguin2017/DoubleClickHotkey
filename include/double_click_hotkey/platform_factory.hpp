#pragma once

#include "double_click_hotkey/platform_binding.hpp"

#include <memory>

namespace double_click_hotkey
{
[[nodiscard]] std::unique_ptr<PlatformBinding> CreatePlatformBinding(bool hide_window_immediately);
} // namespace double_click_hotkey
