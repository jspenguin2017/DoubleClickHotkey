#include "platform/windows/keyboard_sender.hpp"
#include "platform/windows/mouse.hpp"
#include "platform/windows/windows_platform_binding.hpp"

#include <gtest/gtest.h>

#include <windows.h>

#include <array>
#include <cstddef>
#include <initializer_list>
#include <vector>

namespace double_click_hotkey::windows
{
namespace
{
struct PlannedSendInputResult
{
    UINT sent_input_count;
    DWORD error_code;
};

struct SendInputCall
{
    std::vector<INPUT> inputs;
    int input_size;
};

struct SendInputScript
{
    std::vector<PlannedSendInputResult> results;
    std::vector<SendInputCall> calls;
    std::size_t next_result = 0;
};

SendInputScript* active_send_input_script = nullptr;

UINT WINAPI StubSendInput(const UINT input_count, LPINPUT inputs, const int input_size)
{
    if (active_send_input_script == nullptr)
    {
        SetLastError(ERROR_INVALID_STATE);
        return 0;
    }

    SendInputScript& script = *active_send_input_script;
    script.calls.push_back({std::vector<INPUT>(inputs, inputs + input_count), input_size});
    if (script.next_result >= script.results.size())
    {
        SetLastError(ERROR_INVALID_STATE);
        return 0;
    }

    const PlannedSendInputResult result = script.results[script.next_result++];
    SetLastError(result.error_code);
    return result.sent_input_count;
}

void ExpectKeyboardTransition(const INPUT& input, const DWORD flags)
{
    EXPECT_EQ(input.type, INPUT_KEYBOARD);
    EXPECT_EQ(input.ki.wVk, VK_F13);
    EXPECT_EQ(input.ki.dwFlags, flags);
}

void ExpectMouseTransition(const INPUT& input, const DWORD flags)
{
    EXPECT_EQ(input.type, INPUT_MOUSE);
    EXPECT_EQ(input.mi.dwFlags, flags);
}

class InputSendersTest : public testing::Test
{
  protected:
    void SetUp() override
    {
        active_send_input_script = &script_;
    }

    void TearDown() override
    {
        active_send_input_script = nullptr;
    }

    void Plan(const std::initializer_list<PlannedSendInputResult> results)
    {
        script_.results = results;
        script_.calls.clear();
        script_.next_result = 0;
    }

    SendInputScript script_;
};

TEST_F(InputSendersTest, SendsF13AsABalancedSequence)
{
    Plan({{2, ERROR_WRITE_FAULT}});
    KeyboardSender sender(StubSendInput);

    EXPECT_TRUE(sender.SendF13());
    EXPECT_EQ(sender.LastErrorCode(), ERROR_SUCCESS);
    EXPECT_FALSE(sender.LastReleaseErrorCode().has_value());

    ASSERT_EQ(script_.calls.size(), 1);
    EXPECT_EQ(script_.calls[0].input_size, sizeof(INPUT));
    ASSERT_EQ(script_.calls[0].inputs.size(), 2);
    ExpectKeyboardTransition(script_.calls[0].inputs[0], 0);
    ExpectKeyboardTransition(script_.calls[0].inputs[1], KEYEVENTF_KEYUP);
}

TEST_F(InputSendersTest, DoesNotReleaseF13WhenNoKeyDownWasInserted)
{
    Plan({{0, ERROR_WRITE_FAULT}});
    KeyboardSender sender(StubSendInput);

    EXPECT_FALSE(sender.SendF13());
    EXPECT_EQ(sender.LastErrorCode(), ERROR_WRITE_FAULT);
    EXPECT_FALSE(sender.LastReleaseErrorCode().has_value());
    EXPECT_EQ(script_.calls.size(), 1);
}

TEST_F(InputSendersTest, ReleasesF13AfterAKeyDownOnlyPartialSend)
{
    Plan({{1, ERROR_WRITE_FAULT}, {1, ERROR_SUCCESS}});
    KeyboardSender sender(StubSendInput);

    EXPECT_FALSE(sender.SendF13());
    EXPECT_EQ(sender.LastErrorCode(), ERROR_WRITE_FAULT);
    EXPECT_FALSE(sender.LastReleaseErrorCode().has_value());

    ASSERT_EQ(script_.calls.size(), 2);
    ASSERT_EQ(script_.calls[1].inputs.size(), 1);
    EXPECT_EQ(script_.calls[1].input_size, sizeof(INPUT));
    ExpectKeyboardTransition(script_.calls[1].inputs[0], KEYEVENTF_KEYUP);
}

TEST_F(InputSendersTest, PreservesTheF13SendAndReleaseErrors)
{
    Plan({{1, ERROR_WRITE_FAULT}, {0, ERROR_NOT_READY}});
    KeyboardSender sender(StubSendInput);

    EXPECT_FALSE(sender.SendF13());
    EXPECT_EQ(sender.LastErrorCode(), ERROR_WRITE_FAULT);
    ASSERT_TRUE(sender.LastReleaseErrorCode().has_value());
    EXPECT_EQ(*sender.LastReleaseErrorCode(), ERROR_NOT_READY);
}

TEST_F(InputSendersTest, SendsADoubleClickAsBalancedPairs)
{
    Plan({{4, ERROR_WRITE_FAULT}});
    Mouse mouse(StubSendInput);

    EXPECT_TRUE(mouse.DoubleClick());
    EXPECT_EQ(mouse.LastErrorCode(), ERROR_SUCCESS);
    EXPECT_FALSE(mouse.LastReleaseErrorCode().has_value());

    ASSERT_EQ(script_.calls.size(), 1);
    EXPECT_EQ(script_.calls[0].input_size, sizeof(INPUT));
    ASSERT_EQ(script_.calls[0].inputs.size(), 4);
    ExpectMouseTransition(script_.calls[0].inputs[0], MOUSEEVENTF_LEFTDOWN);
    ExpectMouseTransition(script_.calls[0].inputs[1], MOUSEEVENTF_LEFTUP);
    ExpectMouseTransition(script_.calls[0].inputs[2], MOUSEEVENTF_LEFTDOWN);
    ExpectMouseTransition(script_.calls[0].inputs[3], MOUSEEVENTF_LEFTUP);
}

TEST_F(InputSendersTest, DoesNotReleaseTheMouseAfterABalancedPartialSend)
{
    for (const UINT sent_input_count : std::array<UINT, 2>{0, 2})
    {
        SCOPED_TRACE(sent_input_count);
        Plan({{sent_input_count, ERROR_WRITE_FAULT}});
        Mouse mouse(StubSendInput);

        EXPECT_FALSE(mouse.DoubleClick());
        EXPECT_EQ(mouse.LastErrorCode(), ERROR_WRITE_FAULT);
        EXPECT_FALSE(mouse.LastReleaseErrorCode().has_value());
        EXPECT_EQ(script_.calls.size(), 1);
    }
}

TEST_F(InputSendersTest, ReleasesTheMouseAfterAnUnbalancedPartialSend)
{
    for (const UINT sent_input_count : std::array<UINT, 2>{1, 3})
    {
        SCOPED_TRACE(sent_input_count);
        Plan({{sent_input_count, ERROR_WRITE_FAULT}, {1, ERROR_SUCCESS}});
        Mouse mouse(StubSendInput);

        EXPECT_FALSE(mouse.DoubleClick());
        EXPECT_EQ(mouse.LastErrorCode(), ERROR_WRITE_FAULT);
        EXPECT_FALSE(mouse.LastReleaseErrorCode().has_value());

        ASSERT_EQ(script_.calls.size(), 2);
        ASSERT_EQ(script_.calls[1].inputs.size(), 1);
        EXPECT_EQ(script_.calls[1].input_size, sizeof(INPUT));
        ExpectMouseTransition(script_.calls[1].inputs[0], MOUSEEVENTF_LEFTUP);
    }
}

TEST_F(InputSendersTest, PreservesTheMouseSendAndReleaseErrors)
{
    Plan({{3, ERROR_WRITE_FAULT}, {0, ERROR_NOT_READY}});
    Mouse mouse(StubSendInput);

    EXPECT_FALSE(mouse.DoubleClick());
    EXPECT_EQ(mouse.LastErrorCode(), ERROR_WRITE_FAULT);
    ASSERT_TRUE(mouse.LastReleaseErrorCode().has_value());
    EXPECT_EQ(*mouse.LastReleaseErrorCode(), ERROR_NOT_READY);
}

TEST_F(InputSendersTest, ExplainsAnF13InjectionFailureWithoutAnErrorCode)
{
    Plan({{0, ERROR_SUCCESS}});
    WindowsPlatformBinding platform(StubSendInput);

    const PlatformResult result = platform.SendF13();

    EXPECT_EQ(result.status, PlatformResultStatus::failure);
    EXPECT_EQ(result.error_message,
              "Failed to send F13: Windows blocked or otherwise rejected the input without reporting an error code. "
              "An integrity-level mismatch is one possible cause; if the target application is elevated, run Double "
              "Click Hotkey at the same or a higher integrity level.");
}

TEST_F(InputSendersTest, ExplainsADoubleClickInjectionFailureWithoutAnErrorCode)
{
    Plan({{0, ERROR_SUCCESS}});
    WindowsPlatformBinding platform(StubSendInput);

    const PlatformResult result = platform.DoubleClick();

    EXPECT_EQ(result.status, PlatformResultStatus::failure);
    EXPECT_EQ(result.error_message,
              "Failed to send a double-click: Windows blocked or otherwise rejected the input without reporting an "
              "error code. An integrity-level mismatch is one possible cause; if the target application is elevated, "
              "run Double Click Hotkey at the same or a higher integrity level.");
}
} // namespace
} // namespace double_click_hotkey::windows
