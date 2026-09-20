#include "double_click_hotkey/application.hpp"
#include "fake_platform_binding.hpp"

#include <gtest/gtest.h>

#include <stdexcept>

namespace double_click_hotkey
{
using namespace std::chrono_literals;

namespace
{
void EmitFailingEvent(FakePlatformBinding& platform, const EventKind kind)
{
    try
    {
        platform.Emit(kind);
    }
    catch (...)
    {
        // Check before RunService emits quit: adapter cleanup must not mask a controller failure to stop.
        EXPECT_EQ(platform.exit_count, 1);
        EXPECT_EQ(platform.timer, std::nullopt);
        throw;
    }
    ADD_FAILURE() << "Expected event handling to propagate the platform exception";
}
} // namespace

TEST(DelayTest, AcceptsWholeSecondsInRange)
{
    EXPECT_EQ(ParseDelay("1"), 1s);
    EXPECT_EQ(ParseDelay("5"), 5s);
    EXPECT_EQ(ParseDelay("3600"), 3600s);
    EXPECT_EQ(ParseDelay("005"), 5s);
}
TEST(DelayTest, RejectsInvalidOrOverflowingValues)
{
    for (auto text : {"", "0", "000", "3601", "-1", "+5", "1.5", " 5", "5 ", "\t5", "5\n", "5s", "5 0", u8"５",
                      "4294967296", "999999999999999999999"})
        EXPECT_EQ(ParseDelay(text), std::nullopt) << text;
    EXPECT_EQ(ParseDelay(std::string_view("5\0", 2)), std::nullopt);
}
TEST(DelayTest, ParsesOnlyTheSuppliedView)
{
    EXPECT_EQ(ParseDelay(std::string_view("36001", 4)), 3600s);
}
TEST(ApplicationTest, StartsHiddenWithFiveSecondDelay)
{
    FakePlatformBinding p;
    Application app(p);
    EXPECT_EQ(app.Run(), 0);
    EXPECT_EQ(p.run_count, 1);
    EXPECT_EQ(p.present_count, 1);
    EXPECT_EQ(p.visibility, std::vector<bool>{false});
    EXPECT_EQ(p.view.delay_text, "5");
    EXPECT_EQ(p.view.log_text, "");
    EXPECT_EQ(p.view.log_revision, 0U);
    EXPECT_EQ(p.view.removed_lines, 0U);
    EXPECT_EQ(p.view.send_caption, "Send F13");
    EXPECT_TRUE(p.view.delay_enabled);
    EXPECT_TRUE(p.view.send_enabled);
    EXPECT_TRUE(p.scheduled.empty());
    EXPECT_EQ(p.send_count, 0);
    EXPECT_EQ(p.exit_count, 1);
}
TEST(ApplicationTest, DuplicateActivationReturnsSuccessWithoutStartingService)
{
    FakePlatformBinding p;
    p.duplicate = true;
    Application app(p);
    EXPECT_EQ(app.Run(), 0);
    EXPECT_EQ(p.run_count, 1);
    EXPECT_EQ(p.present_count, 0);
    EXPECT_TRUE(p.visibility.empty());
    EXPECT_TRUE(p.scheduled.empty());
    EXPECT_EQ(p.exit_count, 0);
    EXPECT_EQ(p.send_count, 0);
    EXPECT_TRUE(p.errors.empty());
}
TEST(ApplicationTest, ReportsStartupFailure)
{
    FakePlatformBinding p;
    p.service_result = {false, "Setup failed"};
    Application app(p);
    EXPECT_EQ(app.Run(), 1);
    EXPECT_EQ(p.errors, std::vector<std::string>{"Setup failed"});
    EXPECT_EQ(p.present_count, 0);
    EXPECT_TRUE(p.visibility.empty());
    EXPECT_TRUE(p.scheduled.empty());
}
TEST(ApplicationTest, ReportsFailureAfterInitializationAndStops)
{
    FakePlatformBinding p;
    p.run_action = [](auto& f) {
        f.Emit(EventKind::send_requested);
        f.service_result = {false, "Event loop failed"};
    };
    Application app(p);
    EXPECT_EQ(app.Run(), 1);
    EXPECT_EQ(p.view.log_text, "F13 will be sent in 5 seconds. Focus the target application now.\nEvent loop failed");
    EXPECT_EQ(p.errors, std::vector<std::string>{"Event loop failed"});
    EXPECT_EQ(p.exit_count, 1);
    EXPECT_FALSE(p.timer);
    EXPECT_EQ(p.send_count, 0);
}
TEST(ApplicationTest, PresentationFailureCancelsCountdownAndReportsTheError)
{
    FakePlatformBinding p;
    p.run_action = [](auto& f) { EmitFailingEvent(f, EventKind::send_requested); };
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
}
TEST(ApplicationTest, ServiceExceptionsRequestExitAndReportTheError)
{
    class ThrowingService : public FakePlatformBinding
    {
      public:
        PlatformResult RunService(EventHandler) override
        {
            throw std::runtime_error("Service failed");
        }
    } p;
    Application app(p);
    EXPECT_EQ(app.Run(), 1);
    EXPECT_EQ(p.errors, std::vector<std::string>{"Service failed"});
    EXPECT_EQ(p.exit_count, 1);
    EXPECT_EQ(p.present_count, 0);
}
TEST(ApplicationTest, UnknownServiceExceptionsRequestExitAndReportAFallbackError)
{
    class ThrowingService : public FakePlatformBinding
    {
      public:
        PlatformResult RunService(EventHandler) override
        {
            throw 42;
        }
    } p;
    Application app(p);
    EXPECT_EQ(app.Run(), 1);
    EXPECT_EQ(p.errors, std::vector<std::string>{"Unexpected application error."});
    EXPECT_EQ(p.exit_count, 1);
    EXPECT_EQ(p.present_count, 0);
}
TEST(ApplicationTest, UnknownPresentationFailureCancelsCountdownAndReportsAFallbackError)
{
    FakePlatformBinding p;
    p.run_action = [](auto& f) { EmitFailingEvent(f, EventKind::send_requested); };
    p.on_present = [](auto& f) {
        if (!f.view.delay_enabled)
            throw 42;
    };
    Application app(p);
    EXPECT_EQ(app.Run(), 1);
    EXPECT_EQ(p.errors, std::vector<std::string>{"Unexpected application error."});
    EXPECT_EQ(p.exit_count, 1);
    EXPECT_EQ(p.send_count, 0);
    EXPECT_FALSE(p.timer);
}
TEST(ApplicationTest, ShowAndCloseOnlyChangeVisibility)
{
    FakePlatformBinding p;
    p.run_action = [](auto& f) {
        f.Emit(EventKind::show);
        f.Emit(EventKind::hide);
        EXPECT_EQ(f.exit_count, 0);
        EXPECT_TRUE(f.scheduled.empty());
        EXPECT_EQ(f.send_count, 0);
        EXPECT_EQ(f.view.log_text, "");
        EXPECT_EQ(f.view.log_revision, 0U);
        EXPECT_EQ(f.view.delay_text, "5");
        EXPECT_EQ(f.view.send_caption, "Send F13");
        EXPECT_TRUE(f.view.delay_enabled);
        EXPECT_TRUE(f.view.send_enabled);
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
    EXPECT_EQ(p.view.log_revision, 501U);
    std::string expected;
    for (int index = 1; index <= 500; ++index)
    {
        if (index != 1)
            expected += '\n';
        expected += std::to_string(index);
    }
    EXPECT_EQ(p.view.log_text, expected);
}
TEST(ApplicationTest, MultilineAndEmptyDiagnosticsEachAdvanceTheLogRevisionOnce)
{
    FakePlatformBinding p;
    p.run_action = [](auto& f) {
        f.Emit(EventKind::diagnostic, "First\r\nSecond\n");
        EXPECT_EQ(f.view.log_text, "First\nSecond\n");
        EXPECT_EQ(f.view.log_revision, 1U);
        f.Emit(EventKind::diagnostic, "");
        EXPECT_EQ(f.view.log_text, "First\nSecond\n\n");
        EXPECT_EQ(f.view.log_revision, 2U);
        f.Emit(EventKind::show);
        f.Emit(EventKind::hide);
        f.Emit(EventKind::delay_changed, "2");
        f.Emit(EventKind::tick);
        EXPECT_EQ(f.view.log_text, "First\nSecond\n\n");
        EXPECT_EQ(f.view.log_revision, 2U);
        EXPECT_EQ(f.view.removed_lines, 0U);
    };
    Application app(p);
    EXPECT_EQ(app.Run(), 0);
}
TEST(ApplicationTest, InvalidDelayDisablesSendingAndCanBeCorrected)
{
    FakePlatformBinding p;
    p.run_action = [](auto& f) {
        for (auto text : {"", "0", "3601", "5s"})
        {
            SCOPED_TRACE(text);
            f.Emit(EventKind::delay_changed, text);
            EXPECT_EQ(f.view.delay_text, text);
            EXPECT_FALSE(f.view.send_enabled);
            f.Emit(EventKind::send_requested);
            EXPECT_TRUE(f.scheduled.empty());
            EXPECT_EQ(f.send_count, 0);
            EXPECT_EQ(f.view.log_text, "");
            EXPECT_TRUE(f.view.delay_enabled);
            EXPECT_EQ(f.view.send_caption, "Send F13");
        }
        f.Emit(EventKind::delay_changed, "3600");
        EXPECT_EQ(f.view.delay_text, "3600");
        EXPECT_TRUE(f.view.send_enabled);
        f.Emit(EventKind::send_requested);
        EXPECT_EQ(f.view.send_caption, "Sending in 3600 s");
        EXPECT_EQ(f.timer, 1s);
        EXPECT_FALSE(f.view.delay_enabled);
        EXPECT_FALSE(f.view.send_enabled);
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
        EXPECT_EQ(f.view.log_revision, 2U);
        EXPECT_EQ(f.scheduled, (std::vector<std::optional<ElapsedTime>>{1s, 5s, std::nullopt}));
    };
    Application app(p);
    EXPECT_EQ(app.Run(), 0);
    EXPECT_EQ(p.visibility, std::vector<bool>{false});
}
TEST(ApplicationTest, IdleTicksDoNotScheduleOrSendInput)
{
    FakePlatformBinding p;
    p.run_action = [](auto& f) {
        f.now = 1h;
        f.Emit(EventKind::tick);
        EXPECT_TRUE(f.scheduled.empty());
        EXPECT_EQ(f.send_count, 0);
        EXPECT_EQ(f.view.send_caption, "Send F13");
        EXPECT_TRUE(f.view.delay_enabled);
        EXPECT_TRUE(f.view.send_enabled);
        EXPECT_EQ(f.view.log_text, "");
        EXPECT_EQ(f.view.log_revision, 0U);
    };
    Application app(p);
    EXPECT_EQ(app.Run(), 0);
}
TEST(ApplicationTest, CountdownRoundsUpAndSchedulesRelativeToItsStart)
{
    FakePlatformBinding p;
    p.run_action = [](auto& f) {
        f.now = 1250ms;
        f.Emit(EventKind::delay_changed, "2");
        f.Emit(EventKind::send_requested);
        EXPECT_EQ(f.timer, 2250ms);
        EXPECT_EQ(f.view.send_caption, "Sending in 2 s");
        f.now = 2249ms;
        f.Emit(EventKind::tick);
        EXPECT_EQ(f.timer, 2250ms);
        EXPECT_EQ(f.view.send_caption, "Sending in 2 s");
        EXPECT_EQ(f.send_count, 0);
        f.now = 2250ms;
        f.Emit(EventKind::tick);
        EXPECT_EQ(f.timer, 3250ms);
        EXPECT_EQ(f.view.send_caption, "Sending in 1 s");
        EXPECT_EQ(f.send_count, 0);
        f.now = 3249ms;
        f.Emit(EventKind::tick);
        EXPECT_EQ(f.timer, 3250ms);
        EXPECT_EQ(f.view.send_caption, "Sending in 1 s");
        EXPECT_EQ(f.send_count, 0);
        f.now = 3250ms;
        f.Emit(EventKind::tick);
        EXPECT_FALSE(f.timer);
        EXPECT_EQ(f.send_count, 1);
        EXPECT_EQ(f.view.send_caption, "Send F13");
        EXPECT_TRUE(f.view.delay_enabled);
        EXPECT_TRUE(f.view.send_enabled);
    };
    Application app(p);
    EXPECT_EQ(app.Run(), 0);
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
        EXPECT_EQ(f.view.log_text, "F13 will be sent in 2 seconds. Focus the target application now.");
        EXPECT_EQ(f.view.log_revision, 1U);
        EXPECT_EQ(f.scheduled, (std::vector<std::optional<ElapsedTime>>{11s}));
        f.Emit(EventKind::tick);
        EXPECT_EQ(f.timer, 12s);
        EXPECT_EQ(f.send_count, 0);
        f.now = 12s;
        f.Emit(EventKind::tick);
        EXPECT_EQ(f.send_count, 1);
        EXPECT_EQ(f.view.log_text, "F13 will be sent in 2 seconds. Focus the target application now.\nF13 sent.");
    };
    Application app(p);
    EXPECT_EQ(app.Run(), 0);
}
TEST(ApplicationTest, HiddenCountdownContinuesAndOverdueSendRunsOnceAfterResume)
{
    FakePlatformBinding p;
    p.run_action = [](auto& f) {
        f.Emit(EventKind::show);
        f.Emit(EventKind::send_requested);
        f.Emit(EventKind::hide);
        EXPECT_EQ(f.timer, 1s);
        EXPECT_FALSE(f.view.delay_enabled);
        EXPECT_FALSE(f.view.send_enabled);
        f.now = 1h;
        f.Emit(EventKind::tick);
        f.Emit(EventKind::tick);
        EXPECT_EQ(f.send_count, 1);
        EXPECT_FALSE(f.timer);
        EXPECT_TRUE(f.view.delay_enabled);
        EXPECT_TRUE(f.view.send_enabled);
        EXPECT_EQ(f.view.send_caption, "Send F13");
    };
    Application app(p);
    EXPECT_EQ(app.Run(), 0);
    EXPECT_EQ(p.visibility, (std::vector<bool>{false, true, false}));
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
        EXPECT_EQ(f.view.send_caption, "Send F13");
        EXPECT_EQ(f.view.log_text, "F13 will be sent in 5 seconds. Focus the target application now.\nTimer failed");
        EXPECT_EQ(f.scheduled, (std::vector<std::optional<ElapsedTime>>{1s, std::nullopt}));
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
        EXPECT_TRUE(f.view.delay_enabled);
        EXPECT_EQ(f.view.send_caption, "Send F13");
        EXPECT_EQ(f.view.log_text, "F13 will be sent in 5 seconds. Focus the target application now.\nRearm failed");
        EXPECT_FALSE(f.timer);
        EXPECT_EQ(f.scheduled, (std::vector<std::optional<ElapsedTime>>{1s, 2s, std::nullopt}));
        f.now = 10s;
        f.Emit(EventKind::tick);
        EXPECT_EQ(f.send_count, 0);
        f.timer_result = {};
        f.Emit(EventKind::send_requested);
        EXPECT_EQ(f.timer, 11s);
        EXPECT_FALSE(f.view.send_enabled);
        f.now = 15s;
        f.Emit(EventKind::tick);
        EXPECT_EQ(f.send_count, 1);
        EXPECT_FALSE(f.timer);
        EXPECT_TRUE(f.view.send_enabled);
    };
    Application app(p);
    EXPECT_EQ(app.Run(), 0);
}
TEST(ApplicationTest, TimerExceptionStopsBeforePresentingTheCountdown)
{
    class ThrowingTimer : public FakePlatformBinding
    {
      public:
        PlatformResult ScheduleTick(std::optional<ElapsedTime> deadline) override
        {
            (void)FakePlatformBinding::ScheduleTick(deadline);
            throw std::runtime_error("Timer scheduling failed");
        }
    } p;
    p.run_action = [](auto& f) { EmitFailingEvent(f, EventKind::send_requested); };
    Application app(p);
    EXPECT_EQ(app.Run(), 1);
    EXPECT_EQ(p.errors, std::vector<std::string>{"Timer scheduling failed"});
    EXPECT_EQ(p.scheduled, (std::vector<std::optional<ElapsedTime>>{1s}));
    EXPECT_EQ(p.send_count, 0);
    EXPECT_EQ(p.exit_count, 1);
    EXPECT_EQ(p.present_count, 1);
    EXPECT_EQ(p.view.log_text, "");
    EXPECT_EQ(p.view.send_caption, "Send F13");
}
TEST(ApplicationTest, InjectionFailureIsLoggedAndAllowsAnotherAttempt)
{
    FakePlatformBinding p;
    p.send_result = {false, "F13 send failed; release failed"};
    p.run_action = [](auto& f) {
        f.Emit(EventKind::send_requested);
        f.now = 5s;
        f.Emit(EventKind::tick);
        EXPECT_EQ(f.send_count, 1);
        EXPECT_TRUE(f.view.send_enabled);
        EXPECT_TRUE(f.view.delay_enabled);
        EXPECT_FALSE(f.timer);
        EXPECT_EQ(f.view.send_caption, "Send F13");
        EXPECT_EQ(f.view.log_text,
                  "F13 will be sent in 5 seconds. Focus the target application now.\nF13 send failed; release failed");
        f.send_result = {};
        f.Emit(EventKind::delay_changed, "1");
        f.Emit(EventKind::send_requested);
        EXPECT_FALSE(f.view.send_enabled);
        EXPECT_EQ(f.timer, 6s);
        EXPECT_EQ(f.view.send_caption, "Sending in 1 s");
        f.now = 6s;
        f.Emit(EventKind::tick);
        EXPECT_EQ(f.send_count, 2);
        EXPECT_FALSE(f.timer);
        EXPECT_TRUE(f.view.delay_enabled);
        EXPECT_TRUE(f.view.send_enabled);
        EXPECT_EQ(f.view.send_caption, "Send F13");
        EXPECT_EQ(f.view.log_text,
                  "F13 will be sent in 5 seconds. Focus the target application now.\nF13 send failed; release failed\n"
                  "F13 will be sent in 1 seconds. Focus the target application now.\nF13 sent.");
    };
    Application app(p);
    EXPECT_EQ(app.Run(), 0);
}
TEST(ApplicationTest, QuitCancelsCountdownAndIgnoresLaterEvents)
{
    FakePlatformBinding p;
    p.run_action = [](auto& f) {
        f.Emit(EventKind::send_requested);
        const auto presentations = f.present_count;
        f.Emit(EventKind::quit);
        EXPECT_FALSE(f.timer);
        f.now = 10s;
        f.Emit(EventKind::tick);
        f.Emit(EventKind::send_requested);
        f.Emit(EventKind::show);
        f.Emit(EventKind::hide);
        f.Emit(EventKind::delay_changed, "10");
        f.Emit(EventKind::diagnostic, "After quit");
        f.Emit(EventKind::quit);
        EXPECT_EQ(f.send_count, 0);
        EXPECT_EQ(f.exit_count, 1);
        EXPECT_EQ(f.present_count, presentations);
        EXPECT_EQ(f.visibility, std::vector<bool>{false});
        EXPECT_EQ(f.scheduled, (std::vector<std::optional<ElapsedTime>>{1s}));
        EXPECT_EQ(f.view.delay_text, "5");
        EXPECT_EQ(f.view.log_text, "F13 will be sent in 5 seconds. Focus the target application now.");
    };
    Application app(p);
    EXPECT_EQ(app.Run(), 0);
}
TEST(ApplicationTest, DoubleClickDiagnosticsStayHiddenDuringCountdown)
{
    FakePlatformBinding p;
    p.run_action = [](auto& f) {
        f.Emit(EventKind::send_requested);
        f.Emit(EventKind::diagnostic, "Double-click failed; release failed");
        EXPECT_EQ(f.timer, 1s);
        EXPECT_EQ(f.view.log_text, "F13 will be sent in 5 seconds. Focus the target application now.\n"
                                   "Double-click failed; release failed");
        EXPECT_FALSE(f.view.send_enabled);
        EXPECT_EQ(f.send_count, 0);
        EXPECT_EQ(f.visibility, std::vector<bool>{false});
        EXPECT_TRUE(f.errors.empty());
    };
    Application app(p);
    EXPECT_EQ(app.Run(), 0);
}
TEST(ApplicationTest, WorkerFailureCancelsCountdownAndReportsTheError)
{
    FakePlatformBinding p;
    p.run_action = [](auto& f) {
        f.Emit(EventKind::send_requested);
        ASSERT_EQ(f.timer, 1s);
        f.service_result = {false, "Keyboard hook thread exited unexpectedly"};
    };
    Application app(p);
    EXPECT_EQ(app.Run(), 1);
    EXPECT_FALSE(p.timer);
    EXPECT_EQ(p.exit_count, 1);
    EXPECT_EQ(p.send_count, 0);
    EXPECT_EQ(p.errors, (std::vector<std::string>{"Keyboard hook thread exited unexpectedly"}));
}
TEST(ApplicationTest, ReentrantPresentationEventsAreQueuedInOrder)
{
    FakePlatformBinding p;
    bool emitted = false;
    std::vector<std::string> presented_logs;
    p.on_present = [&emitted, &presented_logs](auto& f) {
        presented_logs.push_back(f.view.log_text);
        if (!emitted)
        {
            emitted = true;
            f.Emit(EventKind::diagnostic, "First");
            f.Emit(EventKind::diagnostic, "Second");
            EXPECT_EQ(f.view.log_text, "");
            EXPECT_EQ(f.present_count, 1);
        }
    };
    Application app(p);
    EXPECT_EQ(app.Run(), 0);
    EXPECT_EQ(presented_logs, (std::vector<std::string>{"", "First", "First\nSecond"}));
    EXPECT_EQ(p.view.log_revision, 2U);
}
TEST(ApplicationTest, ReentrantInputEventsDoNotRepeatTheScheduledSend)
{
    class ReentrantSender : public FakePlatformBinding
    {
      public:
        PlatformResult SendF13() override
        {
            EXPECT_FALSE(timer);
            const auto result = FakePlatformBinding::SendF13();
            if (send_count == 1)
            {
                Emit(EventKind::tick);
                Emit(EventKind::diagnostic, "Double-click failed");
                EXPECT_EQ(view.log_text, "F13 will be sent in 5 seconds. Focus the target application now.");
            }
            return result;
        }
    } p;
    p.run_action = [](auto& f) {
        f.Emit(EventKind::send_requested);
        f.now = 5s;
        f.Emit(EventKind::tick);
        EXPECT_EQ(f.send_count, 1);
        EXPECT_EQ(f.scheduled, (std::vector<std::optional<ElapsedTime>>{1s, std::nullopt}));
        EXPECT_EQ(f.view.log_text, "F13 will be sent in 5 seconds. Focus the target application now.\n"
                                   "F13 sent.\nDouble-click failed");
        EXPECT_TRUE(f.view.delay_enabled);
        EXPECT_TRUE(f.view.send_enabled);
    };
    Application app(p);
    EXPECT_EQ(app.Run(), 0);
}
TEST(ApplicationTest, ReentrantSendRequestStartsANewCountdownAfterTheCompletedSend)
{
    class ReentrantSender : public FakePlatformBinding
    {
      public:
        PlatformResult SendF13() override
        {
            const auto result = FakePlatformBinding::SendF13();
            if (send_count == 1)
            {
                Emit(EventKind::delay_changed, "1");
                Emit(EventKind::send_requested);
                EXPECT_EQ(view.delay_text, "5");
                EXPECT_EQ(timer, std::nullopt);
            }
            return result;
        }
    } p;
    p.run_action = [](auto& f) {
        f.Emit(EventKind::send_requested);
        f.now = 5s;
        f.Emit(EventKind::tick);
        EXPECT_EQ(f.send_count, 1);
        EXPECT_EQ(f.view.delay_text, "1");
        EXPECT_EQ(f.view.send_caption, "Sending in 1 s");
        EXPECT_FALSE(f.view.delay_enabled);
        EXPECT_FALSE(f.view.send_enabled);
        EXPECT_EQ(f.timer, 6s);
        EXPECT_EQ(f.view.log_text, "F13 will be sent in 5 seconds. Focus the target application now.\nF13 sent.\n"
                                   "F13 will be sent in 1 seconds. Focus the target application now.");
        f.now = 5999ms;
        f.Emit(EventKind::tick);
        EXPECT_EQ(f.send_count, 1);
        f.now = 6s;
        f.Emit(EventKind::tick);
        EXPECT_EQ(f.send_count, 2);
        EXPECT_EQ(f.timer, std::nullopt);
        EXPECT_EQ(f.view.send_caption, "Send F13");
        EXPECT_TRUE(f.view.delay_enabled);
        EXPECT_TRUE(f.view.send_enabled);
        EXPECT_EQ(f.view.log_text, "F13 will be sent in 5 seconds. Focus the target application now.\nF13 sent.\n"
                                   "F13 will be sent in 1 seconds. Focus the target application now.\nF13 sent.");
        EXPECT_EQ(f.view.log_revision, 4U);
    };
    Application app(p);
    EXPECT_EQ(app.Run(), 0);
}
TEST(ApplicationTest, InputExceptionStopsAndDiscardsReentrantEvents)
{
    class ThrowingSender : public FakePlatformBinding
    {
      public:
        PlatformResult SendF13() override
        {
            (void)FakePlatformBinding::SendF13();
            Emit(EventKind::show);
            Emit(EventKind::diagnostic, "Queued diagnostic");
            Emit(EventKind::send_requested);
            throw std::runtime_error("Input failed");
        }
    } p;
    p.run_action = [](auto& f) {
        f.Emit(EventKind::send_requested);
        f.now = 5s;
        EmitFailingEvent(f, EventKind::tick);
    };
    Application app(p);
    EXPECT_EQ(app.Run(), 1);
    EXPECT_EQ(p.errors, std::vector<std::string>{"Input failed"});
    EXPECT_EQ(p.exit_count, 1);
    EXPECT_EQ(p.send_count, 1);
    EXPECT_EQ(p.scheduled, (std::vector<std::optional<ElapsedTime>>{1s, std::nullopt}));
    EXPECT_EQ(p.visibility, std::vector<bool>{false});
    EXPECT_EQ(p.present_count, 2);
    EXPECT_EQ(p.view.log_text, "F13 will be sent in 5 seconds. Focus the target application now.");
    EXPECT_EQ(p.view.log_revision, 1U);
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
    EXPECT_EQ(p.send_count, 0);
    EXPECT_EQ(p.present_count, 1);
}
TEST(ApplicationTest, ReentrantQuitCancelsAnActiveCountdownAndDiscardsQueuedEvents)
{
    FakePlatformBinding p;
    p.run_action = [](auto& f) {
        f.Emit(EventKind::send_requested);
        EXPECT_EQ(f.exit_count, 1);
        EXPECT_FALSE(f.timer);
    };
    p.on_present = [](auto& f) {
        if (f.timer)
        {
            f.now = 5s;
            f.Emit(EventKind::tick);
            f.Emit(EventKind::diagnostic, "Queued event");
            f.Emit(EventKind::quit);
            EXPECT_FALSE(f.timer);
            EXPECT_EQ(f.exit_count, 1);
            EXPECT_EQ(f.send_count, 0);
        }
    };
    Application app(p);
    EXPECT_EQ(app.Run(), 0);
    EXPECT_EQ(p.exit_count, 1);
    EXPECT_EQ(p.send_count, 0);
    EXPECT_EQ(p.present_count, 2);
    EXPECT_EQ(p.view.log_text, "F13 will be sent in 5 seconds. Focus the target application now.");
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
