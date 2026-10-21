/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#ifndef _PLAYERBOT_BOSSAMBUSHREGISTRY_H
#define _PLAYERBOT_BOSSAMBUSHREGISTRY_H

#include "Define.h"

// Static registry of AMBUSH bosses: bosses that run a loop too fast to be chased,
// so the raid waits for them on the loop instead.
//
// Advance normally walks the party at a loaded boss's LIVE position and the
// engage pulls him wherever the tank catches him. For a boss who RUNS his patrol
// that is never: Baron Geddon's waypoint path 566550 is move_type 1 on all 17
// nodes, his speed_run 2.28571 (~16yd/s against a player's 7), so the ~554yd loop
// takes ~35s and the chase only drags the raid round it behind him — through
// Shazzrah's end, where the loop passes 8.8yd from Shazzrah and the raid enters.
//
// A row turns the chase into an ambush. While the boss is loaded, alive and out of
// combat, Advance walks the party to the row's spot (a point ON his loop, so he
// runs into it and it is on ground he walks) instead of at him, and holds there;
// the ordinary at-boss engage fires when his loop brings him within engage range.
// The wait is bounded (`maxWaitMs`); past it, or once he is in combat, the boss is
// approached the ordinary way again. On the way there the patrolling-boss hold
// (DungeonClearAdvanceAction::TryPatrollingBossHold) keeps the raid from walking
// into him on the stretch between it and the spot.
//
// Mirrors BossPullbackRegistry (which drags a boss to a camp): hand-authored,
// pure, one row per boss.
struct BossAmbush
{
    uint32 mapId{0};
    uint32 bossEntry{0};
    float  x{0.0f}, y{0.0f}, z{0.0f};  // the spot, on his patrol path
    float  arriveRadius{0.0f};          // 2D yd within which the party holds there
    uint32 maxWaitMs{0};                // longest hold at the spot before chasing
};

class BossAmbushRegistry
{
public:
    // The ambush row for (mapId, bossEntry), or nullptr. Pure; linear scan.
    static BossAmbush const* Find(uint32 mapId, uint32 bossEntry);
};

#endif  // _PLAYERBOT_BOSSAMBUSHREGISTRY_H
