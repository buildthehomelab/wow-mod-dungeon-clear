/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#ifndef _PLAYERBOT_DCPARTYHOLDDECISION_H
#define _PLAYERBOT_DCPARTYHOLDDECISION_H

#include <cstdint>

// The pure kernel behind a BossPartyHoldRegistry row (DungeonClearAdvanceAction::
// TryPartyHold is the glue). One evaluation per leader tick over a snapshot:
//
//   Off     — no row in force: the ordinary approach.
//   Walk    — the raid walks to the spot behind the tank (follow-tank as usual).
//   Gather  — the tank stands on the spot and waits for the followers; the spot
//             is already published so the followers walk to it rather than to
//             the tank. Bounded by the row's gather budget.
//   Hold    — gathered (or the budget ran out): the spot stays published and the
//             tank goes on to the boss alone.
//
// `gathered` LATCHES on the leader (DcApproachState::partyHoldGathered, keyed by
// the boss GUID) so a follower that steps off the spot later is recalled by its
// own rung rather than by the tank walking back to stand on the spot again.
namespace DcPartyHoldDecision
{
    enum class Phase : std::uint8_t
    {
        Off = 0,
        Walk,
        Gather,
        Hold,
    };

    struct Inputs
    {
        bool          inForce = false;      // row found and its prerequisites hold
        bool          gathered = false;     // the leader's latch for this boss
        bool          leaderAtSpot = false; // tank inside arriveRadius, on the floor
        std::uint32_t living = 0;           // follower bots alive on the map
        std::uint32_t atSpot = 0;           // ...of which within the gather radius
        std::uint32_t quorumPct = 100;      // DungeonClear.RaidReadyQuorumPct
        std::uint32_t nowMs = 0;
        std::uint32_t gatherSinceMs = 0;    // 0 = the gather clock has not started
        std::uint32_t gatherBudgetMs = 0;   // 0 = wait for the quorum, however long
    };

    struct Verdict
    {
        Phase phase = Phase::Off;
        bool  publish = false;      // followers hold on the spot this tick
        bool  parkLeader = false;   // the tank stops on the spot and owns the tick
        bool  setGathered = false;  // latch `gathered` now
        bool  startClock = false;   // stamp gatherSinceMs = nowMs now
        bool  budgetRanOut = false; // Hold was reached by the budget, not the quorum
    };

    inline bool QuorumMet(std::uint32_t living, std::uint32_t atSpot, std::uint32_t quorumPct)
    {
        if (living == 0)
            return true;
        return atSpot * 100 >= living * quorumPct;
    }

    inline Verdict Decide(Inputs const& in)
    {
        Verdict v;
        if (!in.inForce)
            return v;
        if (in.gathered)
        {
            v.phase = Phase::Hold;
            v.publish = true;
            return v;
        }
        if (!in.leaderAtSpot)
        {
            v.phase = Phase::Walk;
            return v;
        }
        bool const expired = in.gatherBudgetMs != 0 && in.gatherSinceMs != 0 &&
                             in.nowMs - in.gatherSinceMs >= in.gatherBudgetMs;
        if (QuorumMet(in.living, in.atSpot, in.quorumPct) || expired)
        {
            v.phase = Phase::Hold;
            v.publish = true;
            v.setGathered = true;
            v.budgetRanOut = expired && !QuorumMet(in.living, in.atSpot, in.quorumPct);
            return v;
        }
        v.phase = Phase::Gather;
        v.publish = true;
        v.parkLeader = true;
        v.startClock = in.gatherSinceMs == 0;
        return v;
    }
}

#endif  // _PLAYERBOT_DCPARTYHOLDDECISION_H
