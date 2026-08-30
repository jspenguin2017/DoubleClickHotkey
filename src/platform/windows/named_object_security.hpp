#pragma once

#include <windows.h>

#include <sddl.h>

namespace double_click_hotkey::windows
{
class NamedObjectSecurityAttributes
{
  public:
    explicit NamedObjectSecurityAttributes(const wchar_t* const descriptor_definition) noexcept
    {
        if (ConvertStringSecurityDescriptorToSecurityDescriptorW(descriptor_definition, SDDL_REVISION_1, &descriptor_,
                                                                 nullptr) == 0)
        {
            last_error_code_ = GetLastError();
            return;
        }

        attributes_.lpSecurityDescriptor = descriptor_;
    }

    ~NamedObjectSecurityAttributes()
    {
        if (descriptor_ != nullptr)
        {
            static_cast<void>(LocalFree(descriptor_));
        }
    }

    NamedObjectSecurityAttributes(const NamedObjectSecurityAttributes&) = delete;
    NamedObjectSecurityAttributes& operator=(const NamedObjectSecurityAttributes&) = delete;
    NamedObjectSecurityAttributes(NamedObjectSecurityAttributes&&) = delete;
    NamedObjectSecurityAttributes& operator=(NamedObjectSecurityAttributes&&) = delete;

    [[nodiscard]] bool IsValid() const noexcept
    {
        return descriptor_ != nullptr;
    }

    [[nodiscard]] SECURITY_ATTRIBUTES* Get() noexcept
    {
        return &attributes_;
    }

    [[nodiscard]] DWORD LastErrorCode() const noexcept
    {
        return last_error_code_;
    }

  private:
    PSECURITY_DESCRIPTOR descriptor_ = nullptr;
    SECURITY_ATTRIBUTES attributes_{sizeof(SECURITY_ATTRIBUTES), nullptr, FALSE};
    DWORD last_error_code_ = ERROR_SUCCESS;
};
} // namespace double_click_hotkey::windows
