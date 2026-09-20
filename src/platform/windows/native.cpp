#include "platform/windows/native.hpp"

#include <limits>

namespace double_click_hotkey::windows
{
std::wstring ToWide(const std::string_view text)
{
    if (text.empty())
    {
        return {};
    }
    if (text.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
    {
        return L"\uFFFD";
    }
    // Flags=0 replaces malformed input with U+FFFD on the supported Windows baseline.
    const auto length = static_cast<int>(text.size());
    const int count = MultiByteToWideChar(CP_UTF8, 0, text.data(), length, nullptr, 0);
    if (!count)
    {
        return L"\uFFFD";
    }
    std::wstring result(count, L'\0');
    if (!MultiByteToWideChar(CP_UTF8, 0, text.data(), length, result.data(), count))
    {
        return L"\uFFFD";
    }
    return result;
}

std::string ToUtf8(const std::wstring_view text)
{
    if (text.empty())
    {
        return {};
    }
    if (text.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
    {
        return "\xEF\xBF\xBD";
    }
    const auto length = static_cast<int>(text.size());
    const int count = WideCharToMultiByte(CP_UTF8, 0, text.data(), length, nullptr, 0, nullptr, nullptr);
    if (!count)
    {
        return "\xEF\xBF\xBD";
    }
    std::string result(count, '\0');
    if (!WideCharToMultiByte(CP_UTF8, 0, text.data(), length, result.data(), count, nullptr, nullptr))
    {
        return "\xEF\xBF\xBD";
    }
    return result;
}

std::string NativeError(const std::string_view operation, const DWORD error)
{
    wchar_t buffer[1024]{};
    const auto count = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, error, 0,
                                      buffer, 1024, nullptr);
    std::string message(operation);
    message += " (Windows error " + std::to_string(error) + ")";
    if (count)
    {
        message += ": " + ToUtf8(std::wstring_view(buffer, count));
    }
    return message;
}

void Require(const bool success, const std::string_view operation)
{
    if (!success)
    {
        throw std::runtime_error(NativeError(operation));
    }
}

} // namespace double_click_hotkey::windows
