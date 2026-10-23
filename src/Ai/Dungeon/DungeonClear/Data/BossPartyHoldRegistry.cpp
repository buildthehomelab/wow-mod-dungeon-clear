/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "BossPartyHoldRegistry.h"

#include <vector>

namespace
{
    std::vector<BossPartyHold> const& Rows()
    {
        static std::vector<BossPartyHold> const rows = {
            // Blackwing Lair (469): Ebonroc. He walks path 857800 out and back,
            // from his spawn on the 477 tier down his ramp to its foot on the 465
            // tier (node 7, -7414.4, -963.3, 466.5) and back, ~34s a lap; the tank
            // waits for him beside the tank spot (BossAmbushRegistry) and the raid
            // strategy walks him onto it (-7384.6, -954.8).
            //
            // The raid waits on trio B's stand spot (ScriptedPullRegistry row 10,
            // the ground it tagged the Wyrmguards from): 39.8yd from node 7, the
            // nearest point of his path, so he walks past unengaged without seeing
            // them; 30.4yd from the ambush spot and 30.9 from the tank spot, inside
            // the raid spread gate, so the tank's engage is never held on them;
            // 40.5yd from the nearest member of the middle 465 pack (row 8, fought
            // AFTER him — see THE MIDDLE PACK WAITS FOR EBONROC in
            // ScriptedPullRegistry.cpp), which is why the raid cannot use the
            // ledge Flamegor's row uses: that ledge is 12yd from that pack.
            //
            // afterStage 10: trio B (and Krixix) stand 26-40yd from the spot and
            // are dragged off to the east camp by that row first.
            { 469, 14601, -7398.0f, -927.0f, 465.1f, /*arriveRadius*/ 8.0f,
              /*afterStage*/ 10, /*clearRadius*/ 30.0f, /*gatherBudgetMs*/ 45000 },

            // Blackwing Lair (469): Flamegor. He wanders 10yd round his spawn on
            // the 477 tier (-7407.9, -1031.0, 477.4), 36.8-42.8yd from trio A, and
            // the raid strategy walks him down Ebonroc's ramp to the same tank spot
            // — an ~80yd drag that the whole raid used to follow up the ramp.
            //
            // The raid waits on the ledge along the hall's south wall on the 465
            // tier, picked in game (10/23, .gps -7426.5 -966.2 464.9): 68.6yd from
            // Flamegor's spawn and 12.4yd from the foot of Ebonroc's ramp, which
            // by then is empty — Ebonroc is dead, his trio (row 10) and the middle
            // 465 pack (row 8, nearest spawn 12.2yd: Warlock 85775) are dead, and
            // Firemaw's path never comes closer than 47yd. The tank spot is 43.4yd
            // off, so the raid runs in once the tank has him there.
            //
            // afterStage 8: the middle pack. Rows 7/9/10 are already dead by the
            // time Flamegor is next; row 8 is the one fought while he is.
            { 469, 11981, -7426.5f, -966.2f, 464.9f, /*arriveRadius*/ 8.0f,
              /*afterStage*/ 8, /*clearRadius*/ 30.0f, /*gatherBudgetMs*/ 45000 },
        };
        return rows;
    }
}

BossPartyHold const* BossPartyHoldRegistry::Find(uint32 mapId, uint32 bossEntry)
{
    for (BossPartyHold const& r : Rows())
        if (r.mapId == mapId && r.bossEntry == bossEntry)
            return &r;
    return nullptr;
}
