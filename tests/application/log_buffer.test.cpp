#include "double_click_hotkey/log_buffer.hpp"

#include <gtest/gtest.h>

namespace double_click_hotkey
{
TEST(LogBufferTest, StartsEmpty)
{
    LogBuffer log;
    EXPECT_EQ(log.Text(), "");
    EXPECT_EQ(log.LineCount(), 0U);
    EXPECT_EQ(log.RemovedLines(), 0U);
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
    EXPECT_EQ(log.RemovedLines(), 0U);
}
TEST(LogBufferTest, PreservesEmptyLinesAtTerminatorBoundaries)
{
    const struct
    {
        std::string_view input;
        std::string_view expected;
        std::size_t lines;
    } cases[] = {{"", "", 1},         {"\r", "\n", 2},       {"\n", "\n", 2},    {"\r\n", "\n", 2},
                 {"\n\r", "\n\n", 3}, {"\r\r\n", "\n\n", 3}, {"\n\n", "\n\n", 3}};
    for (const auto& test : cases)
    {
        SCOPED_TRACE(std::string(test.input));
        LogBuffer log;
        log.AppendLine(test.input);
        EXPECT_EQ(log.Text(), test.expected);
        EXPECT_EQ(log.LineCount(), test.lines);
        EXPECT_EQ(log.RemovedLines(), 0U);
    }
}
TEST(LogBufferTest, RetainsExactlyFiveHundredNewestLines)
{
    LogBuffer log;
    std::string expected;
    for (int index = 0; index < 500; ++index)
    {
        log.AppendLine(std::to_string(index));
        if (index)
            expected += '\n';
        expected += std::to_string(index);
    }
    EXPECT_EQ(log.LineCount(), 500U);
    EXPECT_EQ(log.RemovedLines(), 0U);
    EXPECT_EQ(log.Text(), expected);
    log.AppendLine("500");
    EXPECT_EQ(log.LineCount(), 500U);
    EXPECT_EQ(log.RemovedLines(), 1U);
    EXPECT_EQ(log.Text(), expected.substr(2) + "\n500");
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
    std::string expected;
    for (int index = 100; index < 600; ++index)
    {
        if (index != 100)
            expected += '\n';
        expected += std::to_string(index);
    }
    EXPECT_EQ(log.Text(), expected);
    log.AppendLine("next");
    EXPECT_EQ(log.Text(), expected.substr(4) + "\nnext");
    EXPECT_EQ(log.LineCount(), 500U);
    EXPECT_EQ(log.RemovedLines(), 102U);
}
TEST(LogBufferTest, CountsBlankLinesWhenTrimming)
{
    LogBuffer log;
    log.AppendLine("old");
    log.AppendLine(std::string(500, '\n'));
    EXPECT_EQ(log.Text(), std::string(499, '\n'));
    EXPECT_EQ(log.LineCount(), 500U);
    EXPECT_EQ(log.RemovedLines(), 2U);
}
TEST(LogBufferTest, CopiesOnlyTheSuppliedViewIncludingEmbeddedNulls)
{
    LogBuffer log;
    std::string message("before\0after\nignored", 20);
    log.AppendLine(std::string_view(message.data(), 12));
    message.assign("changed");
    log.AppendLine("next");
    EXPECT_EQ(log.Text(), std::string("before\0after\nnext", 17));
    EXPECT_EQ(log.LineCount(), 2U);
    EXPECT_EQ(log.RemovedLines(), 0U);
}
TEST(LogBufferTest, PreservesUnicodeAndLongLines)
{
    LogBuffer log;
    const std::string message = u8"F13 → 双击 🖱" + std::string(40000, 'x');
    log.AppendLine(message);
    EXPECT_EQ(log.LineCount(), 1U);
    EXPECT_EQ(log.Text(), message);
    EXPECT_EQ(log.RemovedLines(), 0U);
}
} // namespace double_click_hotkey
