#include "platform/windows/instance_command.hpp"

#include <gtest/gtest.h>

#include <windows.h>

#include <string>

namespace double_click_hotkey::windows
{
namespace
{
class InstanceCommandTest : public testing::Test
{
  protected:
    InstanceCommandTest()
        : channel_base_name_(L"double-click-hotkey-test-" + std::to_wstring(GetCurrentProcessId()) + L"-" +
                             std::to_wstring(GetTickCount64())),
          event_name_(channel_base_name_ + L"-event"), state_name_(channel_base_name_ + L"-state"),
          channel_names_{event_name_.c_str(), state_name_.c_str()}, receiver_(channel_names_)
    {
    }

    void SetUp() override
    {
        ASSERT_TRUE(receiver_.Initialize()) << "error code: " << receiver_.LastErrorCode();
    }

    testing::AssertionResult Send(const InstanceCommand command)
    {
        DWORD error_code = ERROR_SUCCESS;
        if (SendInstanceCommand(command, error_code, channel_names_))
        {
            return testing::AssertionSuccess();
        }
        return testing::AssertionFailure() << "error code: " << error_code;
    }

  private:
    std::wstring channel_base_name_;
    std::wstring event_name_;
    std::wstring state_name_;

  protected:
    InstanceCommandChannelNames channel_names_;
    InstanceCommandReceiver receiver_;
};

TEST_F(InstanceCommandTest, AppliesTheLatestCommandWhenWakeEventsCoalesce)
{
    ASSERT_TRUE(Send(InstanceCommand::hide_window));
    ASSERT_TRUE(Send(InstanceCommand::show_window));

    ASSERT_EQ(WaitForSingleObject(receiver_.EventHandles()[0], 0), WAIT_OBJECT_0);
    const std::optional<InstanceCommand> show_command = receiver_.TakeLatestCommand();
    ASSERT_TRUE(show_command.has_value());
    EXPECT_EQ(*show_command, InstanceCommand::show_window);
    EXPECT_FALSE(receiver_.TakeLatestCommand().has_value());

    ASSERT_TRUE(Send(InstanceCommand::show_window));
    ASSERT_TRUE(Send(InstanceCommand::hide_window));

    ASSERT_EQ(WaitForSingleObject(receiver_.EventHandles()[0], 0), WAIT_OBJECT_0);
    const std::optional<InstanceCommand> hide_command = receiver_.TakeLatestCommand();
    ASSERT_TRUE(hide_command.has_value());
    EXPECT_EQ(*hide_command, InstanceCommand::hide_window);
    EXPECT_FALSE(receiver_.TakeLatestCommand().has_value());
}
} // namespace
} // namespace double_click_hotkey::windows
