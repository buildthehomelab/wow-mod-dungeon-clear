/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#ifndef _PLAYERBOT_BOSSPARTYHOLDREGISTRY_H
#define _PLAYERBOT_BOSSPARTYHOLDREGISTRY_H

#include "Define.h"

// Static registry of PARTY-HOLD bosses: bosses the tank fetches ALONE while the
// rest of the raid waits on a hand-authored spot, out of the way.
//
// The ordinary boss approach walks the whole raid at the boss behind the tank
// (follow-tank), so forty bots stand wherever the tank happens to catch him. For
// a boss the raid strategy then WALKS to a fixed tank spot (mod-playerbots'
// RaidClear.<Map>.<Boss>.TankSpot: Blackwing Lair's Ebonroc and Flamegor) that
// is the wrong place to be twice over — the raid is spread across the ground he
// is dragged over, and for a patroller it is standing inside his loop before the
// tank has him. Flamegor was the worse case: the raid trailed the tank up onto
// the 477 tier and spread round him, 36-43yd from his Wyrmguard trio.
//
// A row turns the approach into a fetch. While the boss is loaded, alive and out
// of combat (and the row's prerequisites hold), Advance first walks the raid to
// the row's spot, waits there for the followers to gather, then PUBLISHES the
// spot (DcRunState::partyHold*) and goes on to the boss alone — the ambush spot
// if he has a BossAmbushRegistry row, his live position otherwise. Every other
// member holds on the spot (DungeonClearPartyHoldTrigger / Action) until the boss
// enters combat, i.e. until the tank has him; from there the fight is the raid
// strategy's, and its own drake-walk hold keeps the raid off him while the tank
// walks him to the tank spot. The spread gate and the pre-boss muster measure
// the raid against the spot rather than the tank, so the tank is free to leave.
//
// Where the spot goes is the whole row: on the boss's floor, on ground the raid
// has fought on, outside every live pack's reach (the row names the stage whose
// pack must be dead, and the leader also refuses the hold while anything hostile
// stands within `clearRadius` of the spot), and — for a patroller — outside his
// loop's reach, since the raid stands there while he walks past unengaged.
//
// Mirrors BossAmbushRegistry / BossPullbackRegistry: hand-authored, pure, one
// row per boss, measured against acore_world and the raid's own logs.
struct BossPartyHold
{
    uint32 mapId{0};
    uint32 bossEntry{0};
    float  x{0.0f}, y{0.0f}, z{0.0f};  // where the followers wait
    float  arriveRadius{0.0f};          // 2D yd within which a member counts as there

    // A ScriptedPullRegistry stage (its `order` on this map) whose pack must be
    // dead before the hold is in force, -1 for none. The spot is in that pack's
    // lap; walking the raid onto it with the pack up is the pull the stage exists
    // to avoid. Same contract as BossAmbush::afterStage.
    int32  afterStage{-1};

    // No live hostile may stand within this of the spot for the hold to be in
    // force (0 = don't check). The stage above covers the pack the row was
    // measured against; this covers whatever else is still standing there on the
    // day — a straggler, a respawn, a pack the route never fought.
    float  clearRadius{0.0f};

    // Longest the leader waits at the spot for the followers to gather before
    // going on without the stragglers. Measured from the tick the leader first
    // stands on the spot; the quorum is DungeonClear.RaidReadyQuorumPct.
    uint32 gatherBudgetMs{0};
};

class BossPartyHoldRegistry
{
public:
    // The party-hold row for (mapId, bossEntry), or nullptr. Pure; linear scan.
    static BossPartyHold const* Find(uint32 mapId, uint32 bossEntry);
};

#endif  // _PLAYERBOT_BOSSPARTYHOLDREGISTRY_H
