/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "BossAmbushRegistry.h"

#include <vector>

namespace
{
    std::vector<BossAmbush> const& Rows()
    {
        static std::vector<BossAmbush> const rows = {
            // Molten Core (409): Baron Geddon. His loop (path 566550, all nodes
            // run) goes from his spawn at the top of his ramp (node 1, 747.5,
            // -981.7, -178.4) down the ramp, round past Shazzrah (passing 8.8yd
            // from him by node 8) and back up; ~35s a lap, ~8s of it by Shazzrah.
            //
            // The spot is on the last leg, node 17 (746.6,-966.6,-181.6) -> node 1,
            // 40% along: Shazzrah is 234yd off and 25yd below, Golemagg 53yd off
            // and 26.5yd below. Idle trash around its height (world DB): Sulfuron's
            // Flameguard 56794 at 56yd (738.5,-1027.9, 3.6yd up), his Lava Elemental
            // 56795 at 63yd (wanders 5yd, 5.3yd up). The ramp's own trash, all
            // Geddon-linked, sits below it: Firelord 56787 at 38yd (8.5yd down),
            // Flameguard 91267 at 50yd (9.3yd down); the party walks past those
            // on the ramp. Magmadar's Ancient Core Hound 56668 starts its patrol
            // 33.5yd south of the spot and walks away south-west.
            //
            // maxWaitMs 90000: two and a half laps.
            { 409, 12056, 747.0f, -972.6f, -180.3f, /*arriveRadius*/ 8.0f,
              /*maxWaitMs*/ 90000 },

            // Blackwing Lair (469): Firemaw. He walks path 845720 out and back,
            // 246yd each way (walk speed, ~200s a lap): from 40yd east of
            // Broodlord's spawn through the floor packs and the Wyrmguard pack, up
            // the ramp past the landing pack, onto the 465 tier and back. Chased,
            // he is caught wherever he happens to be, on top of whatever pack is
            // there; the 10/21 morning run had him join a hall fight 41s in.
            //
            // The spot is waypoint 1, the near end of his path, where he turns.
            // The three floor packs within 50yd of it (door 15.8yd, south 24.8,
            // north 45.2) are the scripted rows that run before him (orders 2-4,
            // ScriptedPullRegistry), and everything else is 80yd+ away: the
            // Wyrmguard pack 83yd, the landing pack 101yd.
            //
            // maxWaitMs 300000: one and a half laps.
            { 469, 11983, -7534.6f, -1036.2f, 448.9f, /*arriveRadius*/ 8.0f,
              /*maxWaitMs*/ 300000 },

            // Blackwing Lair (469): Ebonroc. He walks path 857800 out and back,
            // 86yd each way: from his spawn on the 477 tier (-7358.2, -994.3) down
            // his ramp to its foot on the 465 tier (node 7, -7414.4, -963.3,
            // 466.5), ~35s a leg. Chased (10/22), the tank caught him at the
            // Wyrmguards he walks past and the raid's dynamic camp was still inside
            // his patrol: he walked into the Wyrmguard fight 23s in.
            //
            // The spot is on the 465 floor next to the raid's tank spot for him
            // (RaidClear.BlackwingLair.Ebonroc.TankSpot, -7384.6, -954.8, where
            // the holder walks him), 8.7yd from it toward the foot of his ramp.
            // Not on the tank spot itself: that is 31.0yd from node 7, and the
            // engage hands off at the 30yd BossEngageRangeCap, so he would turn at
            // the foot of the ramp without ever coming into range. From here node
            // 7 is 22.3yd, the nearest point of his path; inside the 6yd arrive
            // circle it is 28.3 at worst. Node 3, up on the 477 tier, is 25.5yd
            // off in plan but 12yd above, a floor the at-boss check measures by
            // path (~80yd round by his ramp), so he is taken at the foot of the
            // ramp, on this floor.
            //
            // The spot is 1.8-8.4yd from the Wyrmguard spawns he passes (formation
            // 84634) and 11.4yd from Krixix: afterStage 10 keeps the raid off it
            // until that row has dragged them to the east 465 camp and killed them.
            //
            // maxWaitMs 150000: two laps.
            { 469, 14601, -7393.0f, -957.0f, 465.1f, /*arriveRadius*/ 6.0f,
              /*maxWaitMs*/ 150000, /*afterStage*/ 10 },
        };
        return rows;
    }
}

BossAmbush const* BossAmbushRegistry::Find(uint32 mapId, uint32 bossEntry)
{
    for (BossAmbush const& r : Rows())
        if (r.mapId == mapId && r.bossEntry == bossEntry)
            return &r;
    return nullptr;
}
