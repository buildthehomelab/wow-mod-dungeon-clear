/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "Ai/Dungeon/DungeonClear/Data/Events/DungeonEventTables.h"
#include "Ai/Dungeon/DungeonClear/Data/Events/RazorfenKraulEvents.h"
#include "Ai/Dungeon/DungeonClear/Settings/DcSettings.h"
#include "Creature.h"
#include "Group.h"
#include "Player.h"
#include "SmartAI.h"
#include "SmartScript.h"

namespace DcRazorfenKraul
{
Creature* OwnedEscort(Player* bot)
{
    if (!bot || bot->GetMapId() != 47 || !bot->IsAlive() ||
        !DcSettings::GetBool(bot, "EnableRazorfenKraulEscort"))
        return nullptr;
    Creature* escortee = bot->FindNearestCreature(4508, 80.0f, true);
    if (!escortee || escortee->GetMap() != bot->GetMap())
        return nullptr;
    SmartAI* ai = dynamic_cast<SmartAI*>(escortee->AI());
    if (!ai || !ai->IsEscorted() || ai->mEscortQuestID != 1144)
        return nullptr;
    ObjectVector const* owners = ai->GetScript()->GetStoredTargetVector(SMART_ESCORT_TARGETS, *escortee);
    if (!owners)
        return nullptr;
    for (WorldObject* object : *owners)
    {
        Player* owner = object ? object->ToPlayer() : nullptr;
        if (!owner || !owner->IsAlive() || owner->GetMap() != bot->GetMap() ||
            !owner->IsWithinDistInMap(escortee, 80.0f) || owner->GetQuestStatus(1144) != QUEST_STATUS_INCOMPLETE)
            continue;
        if (owner == bot || (bot->GetGroup() && owner->GetGroup() == bot->GetGroup()))
            return escortee;
    }
    return nullptr;
}
}

void RegisterRazorfenKraulEvents(std::vector<DungeonEvent>& out)
{
    auto active = [](Player* bot, AiObjectContext*) { return DcRazorfenKraul::OwnedEscort(bot) != nullptr; };
    DungeonEvent event = EventBuilder(47, 1, "Protect Willix's active quest escort")
        .Conditional(active)
        .Repeatable()
        .Persistent()
        .OwnsThePull()
        .StepsOwnMovement()
        .EscortCreature(4508, -1, 0, -1, 5.0f, 18.0f, 8.0f, 80.0f)
        .Build();
    event.steps.back().escortActive = active;
    out.push_back(std::move(event));
}
