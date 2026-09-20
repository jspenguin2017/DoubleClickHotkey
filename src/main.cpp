#include "double_click_hotkey/application.hpp"
#include "double_click_hotkey/platform_factory.hpp"

int main()
{
    auto platform = double_click_hotkey::CreatePlatformBinding();
    double_click_hotkey::Application application(*platform);
    return application.Run();
}
