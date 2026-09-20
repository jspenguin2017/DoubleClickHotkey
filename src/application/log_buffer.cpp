#include "double_click_hotkey/log_buffer.hpp"

namespace double_click_hotkey
{
void LogBuffer::AppendLine(const std::string_view message)
{
    std::size_t start = 0;
    for (std::size_t index = 0; index <= message.size(); ++index)
    {
        if (index == message.size() || message[index] == '\r' || message[index] == '\n')
        {
            lines_.emplace_back(message.substr(start, index - start));
            if (lines_.size() > MaximumLines)
            {
                lines_.pop_front();
                ++removed_lines_;
            }
            if (index < message.size() && message[index] == '\r' && index + 1 < message.size() &&
                message[index + 1] == '\n')
            {
                ++index;
            }
            start = index + 1;
        }
    }
}

std::string LogBuffer::Text() const
{
    std::string result;
    bool first = true;
    for (const auto& line : lines_)
    {
        if (!first)
            result += '\n';
        result += line;
        first = false;
    }
    return result;
}
std::size_t LogBuffer::LineCount() const noexcept
{
    return lines_.size();
}
std::uint64_t LogBuffer::RemovedLines() const noexcept
{
    return removed_lines_;
}
} // namespace double_click_hotkey
