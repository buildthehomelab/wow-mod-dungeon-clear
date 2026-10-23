/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "gtest/gtest.h"

#include "Ai/Dungeon/DungeonClear/Util/DcPartyHoldDecision.h"

// The party-hold kernel (BossPartyHoldRegistry; DungeonClearAdvanceAction::
// TryPartyHold is the glue): the raid waits on a spot while the tank fetches the
// boss. Pins the phase walk — Walk until the tank stands on the spot, Gather
// while the followers close on it (published, tank parked, clock running),
// Hold once the quorum is in or the budget is gone (published, tank free) — and
// that the gathered latch is what keeps the tank from walking back.

namespace
{
    using namespace DcPartyHoldDecision;

    Inputs Base()
    {
        Inputs in;
        in.inForce = true;
        in.gathered = false;
        in.leaderAtSpot = true;
        in.living = 39;
        in.atSpot = 39;
        in.quorumPct = 90;
        in.nowMs = 100000;
        in.gatherSinceMs = 0;
        in.gatherBudgetMs = 45000;
        return in;
    }
}

TEST(DcPartyHoldDecisionTest, NoRowInForceIsTheOrdinaryApproach)
{
    Inputs in = Base();
    in.inForce = false;
    Verdict const v = Decide(in);
    EXPECT_EQ(v.phase, Phase::Off);
    EXPECT_FALSE(v.publish);
    EXPECT_FALSE(v.parkLeader);
    EXPECT_FALSE(v.setGathered);
    EXPECT_FALSE(v.startClock);
}

TEST(DcPartyHoldDecisionTest, TheRaidWalksToTheSpotBehindTheTank)
{
    // Not on the spot yet: nothing is published, so follow-tank carries the raid
    // there; nothing parks the tank.
    Inputs in = Base();
    in.leaderAtSpot = false;
    in.atSpot = 0;
    Verdict const v = Decide(in);
    EXPECT_EQ(v.phase, Phase::Walk);
    EXPECT_FALSE(v.publish);
    EXPECT_FALSE(v.parkLeader);
    EXPECT_FALSE(v.setGathered);
}

TEST(DcPartyHoldDecisionTest, OnTheSpotTheTankParksAndStartsTheClock)
{
    Inputs in = Base();
    in.atSpot = 10;  // most of the raid still walking in
    Verdict const v = Decide(in);
    EXPECT_EQ(v.phase, Phase::Gather);
    EXPECT_TRUE(v.publish) << "the followers walk to the spot now, not to the tank";
    EXPECT_TRUE(v.parkLeader);
    EXPECT_TRUE(v.startClock) << "first gather tick stamps the clock";
    EXPECT_FALSE(v.setGathered);

    // A later tick of the same gather does not restart the clock.
    in.gatherSinceMs = 90000;
    Verdict const later = Decide(in);
    EXPECT_EQ(later.phase, Phase::Gather);
    EXPECT_FALSE(later.startClock);
    EXPECT_TRUE(later.parkLeader);
}

TEST(DcPartyHoldDecisionTest, TheQuorumReleasesTheTank)
{
    Inputs in = Base();
    in.gatherSinceMs = 95000;
    in.atSpot = 36;  // 36/39 = 92% >= 90
    Verdict const v = Decide(in);
    EXPECT_EQ(v.phase, Phase::Hold);
    EXPECT_TRUE(v.publish);
    EXPECT_FALSE(v.parkLeader) << "the tank goes on to the boss";
    EXPECT_TRUE(v.setGathered);
    EXPECT_FALSE(v.budgetRanOut);

    in.atSpot = 35;  // 89.7%: not yet
    Verdict const notYet = Decide(in);
    EXPECT_EQ(notYet.phase, Phase::Gather);
    EXPECT_TRUE(notYet.parkLeader);
}

TEST(DcPartyHoldDecisionTest, TheBudgetReleasesTheTankWithoutTheStragglers)
{
    Inputs in = Base();
    in.atSpot = 20;
    in.gatherSinceMs = 50000;
    in.nowMs = 95000;  // 45s: on the budget
    Verdict const v = Decide(in);
    EXPECT_EQ(v.phase, Phase::Hold);
    EXPECT_TRUE(v.setGathered);
    EXPECT_TRUE(v.budgetRanOut);
    EXPECT_TRUE(v.publish) << "the spot stays published for the stragglers' own rung";

    in.nowMs = 94999;
    EXPECT_EQ(Decide(in).phase, Phase::Gather);

    // No budget: wait for the quorum however long.
    in.gatherBudgetMs = 0;
    in.nowMs = 10000000;
    EXPECT_EQ(Decide(in).phase, Phase::Gather);
}

TEST(DcPartyHoldDecisionTest, TheGatheredLatchKeepsTheTankFromWalkingBack)
{
    // A straggler steps off the spot after the gather: the spot stays published
    // (its own rung recalls it) and the tank is never parked again.
    Inputs in = Base();
    in.gathered = true;
    in.leaderAtSpot = false;
    in.atSpot = 30;
    Verdict const v = Decide(in);
    EXPECT_EQ(v.phase, Phase::Hold);
    EXPECT_TRUE(v.publish);
    EXPECT_FALSE(v.parkLeader);
    EXPECT_FALSE(v.setGathered);
}

TEST(DcPartyHoldDecisionTest, ATankWithNoFollowersIsGatheredAtOnce)
{
    Inputs in = Base();
    in.living = 0;
    in.atSpot = 0;
    Verdict const v = Decide(in);
    EXPECT_EQ(v.phase, Phase::Hold);
    EXPECT_TRUE(v.setGathered);
    EXPECT_FALSE(v.budgetRanOut);
}

TEST(DcPartyHoldDecisionTest, QuorumArithmetic)
{
    EXPECT_TRUE(QuorumMet(40, 36, 90));
    EXPECT_FALSE(QuorumMet(40, 35, 90));
    EXPECT_TRUE(QuorumMet(5, 5, 100));
    EXPECT_FALSE(QuorumMet(5, 4, 100));
    EXPECT_TRUE(QuorumMet(0, 0, 90));
}
