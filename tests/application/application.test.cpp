#include "double_click_hotkey/application.hpp"
#include "fake_platform_binding.hpp"
#include <gtest/gtest.h>
#include <stdexcept>

namespace double_click_hotkey
{
using namespace std::chrono_literals;

TEST(DelayTest, AcceptsWholeSecondsInRange)
{
    EXPECT_EQ(ParseDelay("1"), 1s);
    EXPECT_EQ(ParseDelay("5"), 5s);
    EXPECT_EQ(ParseDelay("3600"), 3600s);
    EXPECT_EQ(ParseDelay("005"), 5s);
}
TEST(DelayTest, RejectsInvalidOrOverflowingValues)
{
    for (auto text : {"", "0", "3601", "-1", "+5", "1.5", " 5", "5 ", "5s", "4294967296", "999999999999999999999"})
        EXPECT_FALSE(ParseDelay(text)) << text;
}
TEST(ApplicationTest, StartsHiddenWithFiveSecondDelay)
{
    FakePlatformBinding p;
    Application app(p);
    EXPECT_EQ(app.Run(), 0);
    EXPECT_EQ(p.visibility, std::vector<bool>{false});
    EXPECT_EQ(p.view.delay_text, "5");
    EXPECT_EQ(p.view.log_text, "");
    EXPECT_TRUE(p.view.send_enabled);
    EXPECT_EQ(p.exit_count, 1);
}
TEST(ApplicationTest, DuplicateActivationReturnsSuccessWithoutStartingService)
{
    FakePlatformBinding p;
    p.duplicate = true;
    Application app(p);
    EXPECT_EQ(app.Run(), 0);
    EXPECT_EQ(p.present_count, 0);
    EXPECT_TRUE(p.errors.empty());
}
TEST(ApplicationTest, ReportsStartupFailure)
{
    FakePlatformBinding p;
    p.service_result = {false, "Setup failed"};
    Application app(p);
    EXPECT_EQ(app.Run(), 1);
    EXPECT_EQ(p.errors, std::vector<std::string>{"Setup failed"});
}
TEST(ApplicationTest, ReportsFailureAfterInitializationAndStops)
{
    FakePlatformBinding p;
    p.run_action = [](auto& f) { f.service_result = {false, "Event loop failed"}; };
    Application app(p);
    EXPECT_EQ(app.Run(), 1);
    EXPECT_EQ(p.view.log_text, "Event loop failed");
    EXPECT_EQ(p.errors, std::vector<std::string>{"Event loop failed"});
    EXPECT_EQ(p.exit_count, 1);
    EXPECT_FALSE(p.handler_);
}
TEST(ApplicationTest, PresentationFailureCancelsCountdownAndDetachesHandler)
{
    FakePlatformBinding p;
    p.run_action = [](auto& f) { f.Emit(EventKind::send_requested); };
    p.on_present = [](auto& f) {
        if (!f.view.delay_enabled)
            throw std::runtime_error("Presentation failed");
    };
    Application app(p);
    EXPECT_EQ(app.Run(), 1);
    EXPECT_EQ(p.errors, std::vector<std::string>{"Presentation failed"});
    EXPECT_EQ(p.exit_count, 1);
    EXPECT_EQ(p.send_count, 0);
    EXPECT_FALSE(p.timer);
    EXPECT_FALSE(p.handler_);
}
TEST(ApplicationTest, ShowAndCloseOnlyChangeVisibility)
{
    FakePlatformBinding p;
    p.run_action = [](auto& f) {
        f.Emit(EventKind::show);
        f.Emit(EventKind::hide);
        EXPECT_EQ(f.exit_count, 0);
    };
    Application app(p);
    EXPECT_EQ(app.Run(), 0);
    EXPECT_EQ(p.visibility, (std::vector<bool>{false, true, false}));
}
TEST(ApplicationTest, LogsErrorsWhileHiddenAndRetainsOnlyNewestLines)
{
    FakePlatformBinding p;
    p.run_action = [](auto& f) {
        for (int index = 0; index < 501; ++index)
            f.Emit(EventKind::diagnostic, std::to_string(index));
    };
    Application app(p);
    EXPECT_EQ(app.Run(), 0);
    EXPECT_EQ(p.visibility, std::vector<bool>{false});
    EXPECT_EQ(p.view.removed_lines, 1U);
    EXPECT_EQ(p.view.log_text.substr(0, 4), "1\n2\n");
}
TEST(ApplicationTest, InvalidDelayDisablesSendingAndCanBeCorrected)
{
    FakePlatformBinding p;
    p.run_action = [](auto& f) {
        f.Emit(EventKind::delay_changed, "");
        EXPECT_FALSE(f.view.send_enabled);
        f.Emit(EventKind::send_requested);
        EXPECT_FALSE(f.timer);
        f.Emit(EventKind::delay_changed, "3600");
        EXPECT_TRUE(f.view.send_enabled);
        f.Emit(EventKind::send_requested);
        EXPECT_EQ(f.view.send_caption, "Sending in 3600 s");
    };
    Application app(p);
    EXPECT_EQ(app.Run(), 0);
    EXPECT_EQ(p.send_count, 0);
}
TEST(ApplicationTest, SendsOnceAtDeadlineWithoutBlocking)
{
    FakePlatformBinding p;
    p.run_action = [](auto& f) {
        f.Emit(EventKind::send_requested);
        EXPECT_EQ(f.timer, 1s);
        EXPECT_FALSE(f.view.delay_enabled);
        EXPECT_FALSE(f.view.send_enabled);
        EXPECT_EQ(f.view.send_caption, "Sending in 5 s");
        EXPECT_EQ(f.send_count, 0);
        f.now = 4999ms;
        f.Emit(EventKind::tick);
        EXPECT_EQ(f.send_count, 0);
        EXPECT_EQ(f.timer, 5s);
        EXPECT_EQ(f.view.send_caption, "Sending in 1 s");
        f.now = 5s;
        f.Emit(EventKind::tick);
        f.Emit(EventKind::tick);
        EXPECT_EQ(f.send_count, 1);
        EXPECT_FALSE(f.timer);
        EXPECT_TRUE(f.view.delay_enabled);
        EXPECT_TRUE(f.view.send_enabled);
        EXPECT_EQ(f.view.send_caption, "Send F13");
        EXPECT_EQ(f.view.log_text, "F13 will be sent in 5 seconds. Focus the target application now.\nF13 sent.");
    };
    Application app(p);
    EXPECT_EQ(app.Run(), 0);
    EXPECT_EQ(p.visibility, std::vector<bool>{false});
}
TEST(ApplicationTest, CapturesSelectedDelayAndIgnoresRepeatedRequests)
{
    FakePlatformBinding p;
    p.run_action = [](auto& f) {
        f.now = 10s;
        f.Emit(EventKind::delay_changed, "2");
        f.Emit(EventKind::send_requested);
        f.now = 11s;
        f.Emit(EventKind::delay_changed, "100");
        f.Emit(EventKind::send_requested);
        EXPECT_EQ(f.view.delay_text, "2");
        f.Emit(EventKind::tick);
        EXPECT_EQ(f.timer, 12s);
        f.now = 12s;
        f.Emit(EventKind::tick);
        EXPECT_EQ(f.send_count, 1);
    };
    Application app(p);
    EXPECT_EQ(app.Run(), 0);
}
TEST(ApplicationTest, HiddenCountdownContinuesAndOverdueSendRunsOnceAfterResume)
{
    FakePlatformBinding p;
    p.run_action = [](auto& f) {
        f.Emit(EventKind::send_requested);
        f.Emit(EventKind::hide);
        f.now = 1h;
        f.Emit(EventKind::tick);
        f.Emit(EventKind::tick);
        EXPECT_EQ(f.send_count, 1);
    };
    Application app(p);
    EXPECT_EQ(app.Run(), 0);
    EXPECT_EQ(p.visibility, (std::vector<bool>{false, false}));
}
TEST(ApplicationTest, TimerFailureRestoresControlsWithoutSending)
{
    FakePlatformBinding p;
    p.timer_result = {false, "Timer failed"};
    p.run_action = [](auto& f) {
        f.Emit(EventKind::send_requested);
        EXPECT_TRUE(f.view.send_enabled);
        EXPECT_TRUE(f.view.delay_enabled);
        EXPECT_FALSE(f.timer);
        EXPECT_NE(f.view.log_text.find("Timer failed"), std::string::npos);
        f.now = 10s;
        f.Emit(EventKind::tick);
        EXPECT_EQ(f.send_count, 0);
    };
    Application app(p);
    EXPECT_EQ(app.Run(), 0);
}
TEST(ApplicationTest, TimerRearmFailureCancelsThePendingSend)
{
    FakePlatformBinding p;
    p.run_action = [](auto& f) {
        f.Emit(EventKind::send_requested);
        f.timer_result = {false, "Rearm failed"};
        f.now = 1s;
        f.Emit(EventKind::tick);
        EXPECT_TRUE(f.view.send_enabled);
        f.now = 10s;
        f.Emit(EventKind::tick);
        EXPECT_EQ(f.send_count, 0);
    };
    Application app(p);
    EXPECT_EQ(app.Run(), 0);
}
TEST(ApplicationTest, InjectionFailureIsLoggedAndAllowsAnotherAttempt)
{
    FakePlatformBinding p;
    p.send_result = {false, "F13 send failed; release failed"};
    p.run_action = [](auto& f) {
        f.Emit(EventKind::send_requested);
        f.now = 5s;
        f.Emit(EventKind::tick);
        EXPECT_TRUE(f.view.send_enabled);
        EXPECT_NE(f.view.log_text.find("F13 send failed; release failed"), std::string::npos);
        f.Emit(EventKind::send_requested);
        EXPECT_FALSE(f.view.send_enabled);
    };
    Application app(p);
    EXPECT_EQ(app.Run(), 0);
}
TEST(ApplicationTest, QuitCancelsCountdownAndIgnoresLaterEvents)
{
    FakePlatformBinding p;
    p.run_action = [](auto& f) {
        f.Emit(EventKind::send_requested);
        f.Emit(EventKind::quit);
        EXPECT_FALSE(f.timer);
        f.now = 10s;
        f.Emit(EventKind::tick);
        f.Emit(EventKind::send_requested);
        f.Emit(EventKind::hotkey_pressed);
        EXPECT_EQ(f.send_count, 0);
        EXPECT_EQ(f.click_count, 0);
        EXPECT_EQ(f.exit_count, 1);
    };
    Application app(p);
    EXPECT_EQ(app.Run(), 0);
}
TEST(ApplicationTest, DoubleClicksOncePerPressEvenDuringCountdown)
{
    FakePlatformBinding p;
    p.run_action = [](auto& f) {
        f.Emit(EventKind::send_requested);
        f.Emit(EventKind::hotkey_released);
        f.Emit(EventKind::hotkey_pressed);
        f.Emit(EventKind::hotkey_pressed);
        f.Emit(EventKind::hotkey_released);
        f.Emit(EventKind::hotkey_pressed);
        EXPECT_EQ(f.click_count, 2);
    };
    Application app(p);
    EXPECT_EQ(app.Run(), 0);
}
TEST(ApplicationTest, DoubleClickFailureDoesNotShowHiddenWindow)
{
    FakePlatformBinding p;
    p.click_result = {false, "Double-click failed"};
    p.run_action = [](auto& f) { f.Emit(EventKind::hotkey_pressed); };
    Application app(p);
    EXPECT_EQ(app.Run(), 0);
    EXPECT_EQ(p.view.log_text, "Double-click failed");
    EXPECT_EQ(p.visibility, std::vector<bool>{false});
}
TEST(ApplicationTest, ReentrantPresentationEventsAreQueued)
{
    FakePlatformBinding p;
    bool emitted = false;
    p.on_present = [&emitted](auto& f) {
        if (!emitted)
        {
            emitted = true;
            f.Emit(EventKind::diagnostic, "Nested event");
            EXPECT_TRUE(f.view.log_text.empty());
        }
    };
    Application app(p);
    EXPECT_EQ(app.Run(), 0);
    EXPECT_EQ(p.view.log_text, "Nested event");
}
TEST(ApplicationTest, ReentrantQuitCancelsImmediately)
{
    FakePlatformBinding p;
    p.on_present = [](auto& f) {
        f.Emit(EventKind::send_requested);
        f.Emit(EventKind::quit);
        EXPECT_EQ(f.exit_count, 1);
    };
    Application app(p);
    EXPECT_EQ(app.Run(), 0);
    EXPECT_EQ(p.exit_count, 1);
    EXPECT_TRUE(p.scheduled.empty());
}
TEST(ApplicationTest, DelayResetsInANewApplication)
{
    FakePlatformBinding p;
    p.run_action = [](auto& f) { f.Emit(EventKind::delay_changed, "25"); };
    Application first(p);
    EXPECT_EQ(first.Run(), 0);
    EXPECT_EQ(p.view.delay_text, "25");
    FakePlatformBinding other;
    Application second(other);
    EXPECT_EQ(second.Run(), 0);
    EXPECT_EQ(other.view.delay_text, "5");
}
} // namespace double_click_hotkey
