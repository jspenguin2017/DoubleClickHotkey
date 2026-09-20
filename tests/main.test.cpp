#include "application/fake_platform_binding.hpp"
#include "double_click_hotkey/platform_factory.hpp"
#include <gtest/gtest.h>

int DoubleClickHotkeyMain();

namespace double_click_hotkey
{
namespace
{
PlatformResult service_result;
std::vector<std::string> lifecycle;

class EntrypointPlatformBinding : public FakePlatformBinding
{
  public:
    ~EntrypointPlatformBinding() override
    {
        lifecycle.emplace_back("destroyed");
    }
    PlatformResult RunService(EventHandler handler) override
    {
        lifecycle.emplace_back("run");
        return FakePlatformBinding::RunService(std::move(handler));
    }
};

class MainTest : public testing::Test
{
  protected:
    void SetUp() override
    {
        service_result = {};
        lifecycle.clear();
    }
    void TearDown() override
    {
        service_result = {};
        lifecycle.clear();
    }
};
} // namespace
std::unique_ptr<PlatformBinding> CreatePlatformBinding()
{
    lifecycle.emplace_back("created");
    auto fake = std::make_unique<EntrypointPlatformBinding>();
    fake->service_result = service_result;
    fake->run_action = [](auto&) { lifecycle.emplace_back("initialized"); };
    return fake;
}
TEST_F(MainTest, RunsThePlatformNeutralEntrypointAndReleasesItsBinding)
{
    EXPECT_EQ(DoubleClickHotkeyMain(), 0);
    EXPECT_EQ(lifecycle, (std::vector<std::string>{"created", "run", "initialized", "destroyed"}));
}
TEST_F(MainTest, ReturnsApplicationFailureAndReleasesItsBinding)
{
    service_result = {false, "Initialization failed"};
    EXPECT_EQ(DoubleClickHotkeyMain(), 1);
    EXPECT_EQ(lifecycle, (std::vector<std::string>{"created", "run", "destroyed"}));
}
} // namespace double_click_hotkey
