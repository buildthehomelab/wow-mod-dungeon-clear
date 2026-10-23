/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "Ai/Dungeon/DungeonClear/Data/Events/DungeonEventTables.h"
#include "Ai/Dungeon/DungeonClear/Settings/DcSettings.h"
#include <utility>

#include "Creature.h"
#include "GameObject.h"
#include "Player.h"

namespace
{
constexpr uint32 GO_AKU_MAI_DOOR = 21117;
constexpr uint32 NPC_KELRIS = 4832;
constexpr float ROOM_SCAN = 120.0f;

bool BlackfathomFiresDue(Player* bot, AiObjectContext* /*context*/)
{
    if (!bot || bot->GetMapId() != 48 ||
        !DcSettings::GetBool(ObjectGuid::Empty, "EnableBlackfathomFireEvent"))
        return false;
    // The nearby door and fires keep Kelris' room loaded for the prerequisite check.
    GameObject* door = bot->FindNearestGameObject(GO_AKU_MAI_DOOR, 60.0f);
    return door && door->GetGoState() == GO_STATE_READY &&
           !bot->FindNearestCreature(NPC_KELRIS, ROOM_SCAN, true);
}
}

void RegisterBlackfathomEvents(std::vector<DungeonEvent>& out)
{
    auto event = EventBuilder(48, 1, "Fires of Aku'mai")
                     .Conditional(&BlackfathomFiresDue)
                     .Persistent()
                     .PanelAfterBoss(NPC_KELRIS)
                     .CompleteWhenGOState(GO_AKU_MAI_DOOR, GO_STATE_ACTIVE, ROOM_SCAN)
                     .MoveTo(-818.7f, -164.5f, -25.8f, 8.0f)
                     .ClearRadius(-818.7f, -164.5f, -25.8f, 65.0f, 12.0f)
                     .Timeout(180000);
    // UseGO skips already-active fires, including after a combat gap or resumed run.
    // Each synchronous summon group must be dead before the next fire is touched.
    for (auto const& [fire, creature] : {std::pair<uint32, uint32>{21118, 4825},
                                      {21119, 4977}, {21120, 4823}, {21121, 4978}})
    {
        event.UseGO(fire, ROOM_SCAN)
             .WaitForGOState(fire, GO_STATE_ACTIVE, 10000, ROOM_SCAN)
             .KillCreatureEngage(creature, 1, ROOM_SCAN)
             .Timeout(180000);
    }
    out.push_back(event.WaitForGOState(GO_AKU_MAI_DOOR, GO_STATE_ACTIVE, 30000, ROOM_SCAN).Build());
}
