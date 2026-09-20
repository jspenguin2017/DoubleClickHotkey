#pragma once

#include <cstdint>
#include <deque>
#include <string>
#include <string_view>

namespace double_click_hotkey
{
class LogBuffer
{
  public:
    static constexpr std::size_t MaximumLines = 500;
    // Like WriteLine: append the message plus a line terminator. A trailing terminator in the message adds a blank
    // line.
    void AppendLine(std::string_view message);
    [[nodiscard]] std::string Text() const;
    [[nodiscard]] std::size_t LineCount() const noexcept;
    [[nodiscard]] std::uint64_t RemovedLines() const noexcept;

  private:
    std::deque<std::string> lines_;
    std::uint64_t removed_lines_ = 0;
};
} // namespace double_click_hotkey
