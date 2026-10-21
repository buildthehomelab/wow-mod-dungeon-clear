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
