#include "double_click_hotkey/log_buffer.hpp"
#include <gtest/gtest.h>

namespace double_click_hotkey
{
TEST(LogBufferTest, StartsEmpty)
{
    LogBuffer log;
    EXPECT_EQ(log.Text(), "");
    EXPECT_EQ(log.LineCount(), 0U);
}
TEST(LogBufferTest, NormalizesTerminatorsAndPreservesBlankLines)
{
    LogBuffer log;
    log.AppendLine("one\r\ntwo\rthree\n\nfive\n");
    EXPECT_EQ(log.Text(), "one\ntwo\nthree\n\nfive\n");
    EXPECT_EQ(log.LineCount(), 6U);
    log.AppendLine("");
    EXPECT_EQ(log.LineCount(), 7U);
    EXPECT_EQ(log.Text(), "one\ntwo\nthree\n\nfive\n\n");
}
TEST(LogBufferTest, RetainsExactlyFiveHundredNewestLines)
{
    LogBuffer log;
    for (int index = 0; index < 500; ++index)
        log.AppendLine(std::to_string(index));
    EXPECT_EQ(log.LineCount(), 500U);
    EXPECT_EQ(log.RemovedLines(), 0U);
    log.AppendLine("500");
    EXPECT_EQ(log.LineCount(), 500U);
    EXPECT_EQ(log.RemovedLines(), 1U);
    EXPECT_EQ(log.Text().substr(0, 4), "1\n2\n");
    EXPECT_EQ(log.Text().substr(log.Text().size() - 7), "499\n500");
}
TEST(LogBufferTest, TrimsOversizedMultilineAppend)
{
    LogBuffer log;
    log.AppendLine("old");
    std::string batch;
    for (int index = 0; index < 600; ++index)
    {
        if (index)
            batch += '\n';
        batch += std::to_string(index);
    }
    log.AppendLine(batch);
    EXPECT_EQ(log.LineCount(), 500U);
    EXPECT_EQ(log.RemovedLines(), 101U);
    EXPECT_EQ(log.Text().substr(0, 7), "100\n101");
    EXPECT_EQ(log.Text().substr(log.Text().size() - 3), "599");
}
TEST(LogBufferTest, PreservesUnicodeAndLongLines)
{
    LogBuffer log;
    const std::string message = u8"F13 → 双击 🖱" + std::string(40000, 'x');
    log.AppendLine(message);
    EXPECT_EQ(log.LineCount(), 1U);
    EXPECT_EQ(log.Text(), message);
}
} // namespace double_click_hotkey
