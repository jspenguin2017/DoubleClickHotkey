#include "double_click_hotkey/hotkey_policy.hpp"

#include <gtest/gtest.h>

namespace double_click_hotkey
{
namespace
{
constexpr HotkeyEvent Press{KeyTransition::pressed};
constexpr HotkeyEvent Release{KeyTransition::released};
constexpr HotkeyEvent OwnPress{KeyTransition::pressed, true, true};
constexpr HotkeyEvent OwnRelease{KeyTransition::released, true, true};

void ExpectDecision(HotkeyPolicy& policy, const HotkeyEvent& event, const bool suppress, const bool double_click)
{
    const auto decision = policy.Handle(event);
    EXPECT_EQ(decision.suppress, suppress);
    EXPECT_EQ(decision.double_click, double_click);
}
} // namespace

TEST(HotkeyPolicyTest, SuppressesBothTransitionsAndRequestsOneClickPerPress)
{
    HotkeyPolicy policy;
    ExpectDecision(policy, Press, true, true);
    ExpectDecision(policy, Release, true, false);
    ExpectDecision(policy, Press, true, true);
    ExpectDecision(policy, Release, true, false);
}

TEST(HotkeyPolicyTest, SuppressesRepeatsWithoutRequestingMoreClicks)
{
    HotkeyPolicy policy;
    ExpectDecision(policy, Press, true, true);
    for (int repeat = 0; repeat < 20; ++repeat)
        ExpectDecision(policy, Press, true, false);
    ExpectDecision(policy, Release, true, false);
    ExpectDecision(policy, Press, true, true);
}

TEST(HotkeyPolicyTest, PassesUnmatchedReleasesThrough)
{
    HotkeyPolicy policy;
    ExpectDecision(policy, Release, false, false);
    ExpectDecision(policy, Press, true, true);
    ExpectDecision(policy, Release, true, false);
    ExpectDecision(policy, Release, false, false);
}

TEST(HotkeyPolicyTest, PassesAKeyHeldAtStartupThroughUntilItsRelease)
{
    HotkeyPolicy policy(true);
    ExpectDecision(policy, Press, false, false);
    ExpectDecision(policy, Press, false, false);
    ExpectDecision(policy, Release, false, false);
    ExpectDecision(policy, Press, true, true);
    ExpectDecision(policy, Release, true, false);
}

TEST(HotkeyPolicyTest, OwnInjectedInputPassesThroughWithoutEstablishingAPhysicalPress)
{
    HotkeyPolicy policy;
    ExpectDecision(policy, OwnPress, false, false);
    ExpectDecision(policy, OwnRelease, false, false);
    ExpectDecision(policy, OwnRelease, false, false); // Compensating releases also carry the tag.
    ExpectDecision(policy, Release, false, false);
    ExpectDecision(policy, Press, true, true);
}

TEST(HotkeyPolicyTest, OwnInjectedInputDoesNotReleaseAPhysicalHold)
{
    HotkeyPolicy policy;
    ExpectDecision(policy, Press, true, true);
    ExpectDecision(policy, OwnPress, false, false);
    ExpectDecision(policy, Press, true, false);
    ExpectDecision(policy, OwnRelease, false, false);
    ExpectDecision(policy, Press, true, false);
    ExpectDecision(policy, OwnRelease, false, false);
    ExpectDecision(policy, Press, true, false);
    ExpectDecision(policy, Release, true, false);
    ExpectDecision(policy, Press, true, true);
}

TEST(HotkeyPolicyTest, OwnInjectedInputDoesNotEndStartupPassthrough)
{
    HotkeyPolicy policy(true);
    ExpectDecision(policy, OwnPress, false, false);
    ExpectDecision(policy, Press, false, false);
    ExpectDecision(policy, OwnRelease, false, false);
    ExpectDecision(policy, Press, false, false);
    ExpectDecision(policy, Release, false, false);
    ExpectDecision(policy, Press, true, true);
}

TEST(HotkeyPolicyTest, PhysicalPressBetweenOurInjectedTransitionsStillTriggers)
{
    HotkeyPolicy policy;
    ExpectDecision(policy, OwnPress, false, false);
    ExpectDecision(policy, Press, true, true);
    ExpectDecision(policy, OwnRelease, false, false);
    ExpectDecision(policy, Press, true, false);
    ExpectDecision(policy, Release, true, false);
}

TEST(HotkeyPolicyTest, PhysicalReleaseBetweenOurInjectedTransitionsStillReleases)
{
    HotkeyPolicy policy;
    ExpectDecision(policy, Press, true, true);
    ExpectDecision(policy, OwnPress, false, false);
    ExpectDecision(policy, Release, true, false);
    ExpectDecision(policy, OwnRelease, false, false);
    ExpectDecision(policy, Press, true, true);
}

TEST(HotkeyPolicyTest, OtherInjectedInputStillTriggersAndSuppressesRepeats)
{
    HotkeyPolicy policy;
    const HotkeyEvent other_press{KeyTransition::pressed, true, false};
    const HotkeyEvent other_release{KeyTransition::released, true, false};
    ExpectDecision(policy, other_press, true, true);
    ExpectDecision(policy, other_press, true, false);
    ExpectDecision(policy, OwnRelease, false, false);
    ExpectDecision(policy, other_release, true, false);
    ExpectDecision(policy, other_press, true, true);
}

TEST(HotkeyPolicyTest, TagMatchWithoutInjectedFlagDoesNotBypassTheHotkey)
{
    HotkeyPolicy policy;
    ExpectDecision(policy, {KeyTransition::pressed, false, true}, true, true);
    ExpectDecision(policy, {KeyTransition::released, false, true}, true, false);
}
} // namespace double_click_hotkey
