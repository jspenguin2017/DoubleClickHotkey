#pragma once

#include "double_click_hotkey/platform_binding.hpp"

#include <memory>

namespace double_click_hotkey
{
[[nodiscard]] std::unique_ptr<PlatformBinding> CreatePlatformBinding();
} // namespace double_click_hotkey
