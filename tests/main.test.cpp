#include "application/fake_platform_binding.hpp"
#include "double_click_hotkey/platform_factory.hpp"
#include <gtest/gtest.h>

int DoubleClickHotkeyMain();

namespace double_click_hotkey
{
namespace
{
PlatformResult service_result;
int created = 0;
int initialized = 0;
} // namespace
std::unique_ptr<PlatformBinding> CreatePlatformBinding()
{
    ++created;
    auto fake = std::make_unique<FakePlatformBinding>();
    fake->service_result = service_result;
    fake->run_action = [](auto&) { ++initialized; };
    return fake;
}
TEST(MainTest, RunsThePlatformNeutralArgumentFreeEntrypoint)
{
    service_result = {};
    created = initialized = 0;
    EXPECT_EQ(DoubleClickHotkeyMain(), 0);
    EXPECT_EQ(created, 1);
    EXPECT_EQ(initialized, 1);
}
TEST(MainTest, ReturnsApplicationFailure)
{
    service_result = {false, "Initialization failed"};
    created = initialized = 0;
    EXPECT_EQ(DoubleClickHotkeyMain(), 1);
    EXPECT_EQ(created, 1);
    EXPECT_EQ(initialized, 0);
}
} // namespace double_click_hotkey
