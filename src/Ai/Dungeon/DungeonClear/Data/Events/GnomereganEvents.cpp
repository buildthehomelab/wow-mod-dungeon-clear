/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "Ai/Dungeon/DungeonClear/Data/Events/DungeonEventTables.h"
#include "Ai/Dungeon/DungeonClear/Data/Events/DungeonRosterBuilders.h"
#include "DBCStores.h"

namespace
{
constexpr uint32 MAP_GNOMEREGAN = 90;
constexpr uint32 ENCOUNTER_GRUBBIS = 379;
constexpr uint32 NPC_EMI = 7998;
constexpr uint32 NPC_GRUBBIS = 7361;
constexpr float EMI_X = -514.935f;
constexpr float EMI_Y = -138.544f;
constexpr float EMI_Z = -152.399f;

DungeonEncounterEntry const* GrubbisEncounter()
{
    DungeonEncounterEntry const* entry = sDungeonEncounterStore.LookupEntry(ENCOUNTER_GRUBBIS);
    return entry && entry->mapId == MAP_GNOMEREGAN && entry->encounterIndex < 32 ? entry : nullptr;
}
}

void RegisterGnomereganEvents(std::vector<DungeonEvent>& out)
{
    DungeonEncounterEntry const* encounter = GrubbisEncounter();
    if (!encounter)
        return;
    // Grubbis is summoned by Emi's final wave, not a fixed creature spawn. Stay
    // with the escort through his death: seeing him spawn is not completion.
    out.push_back(EventBuilder(MAP_GNOMEREGAN, 1, "Protect Blastmaster Emi Shortfuse")
        .Anchored(encounter->encounterIndex)
        .Persistent()
        .MoveTo(EMI_X, EMI_Y, EMI_Z, 5.0f)
        .EscortCreature(NPC_EMI, 0, 0, static_cast<int32>(encounter->encounterIndex),
                        5.0f, 25.0f, 10.0f, 100.0f)
        .Build());
}

void RegisterGnomereganRoster(std::vector<BossRosterPatch>& out)
{
    DungeonEncounterEntry const* encounter = GrubbisEncounter();
    if (!encounter)
        return;
    using namespace DcRoster;
    BossRosterPatch patch;
    patch.mapId = MAP_GNOMEREGAN;
    patch.remove = {NPC_GRUBBIS};
    patch.add = {MakeObjective(OBJ(1), encounter->encounterIndex, MAP_GNOMEREGAN,
        "Protect Blastmaster Emi Shortfuse", EMI_X, EMI_Y, EMI_Z, 18.0f, 0, 0, 1)};
    out.push_back(std::move(patch));
}
