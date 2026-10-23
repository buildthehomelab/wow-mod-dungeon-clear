#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <string>
#include <utility>
#include <vector>
using uint32 = uint32_t;
constexpr uint32 GO_STATE_READY = 1, GO_STATE_ACTIVE = 0;
struct ObjectGuid { static ObjectGuid const Empty; };
ObjectGuid const ObjectGuid::Empty{};
struct AiObjectContext {};
struct Creature {};
struct GameObject { uint32 state = GO_STATE_READY; uint32 GetGoState() { return state; } };
struct Player
{
    uint32 map = 48; bool nearbyDoor = true, kelrisAlive = false;
    GameObject door; Creature kelris;
    uint32 GetMapId() { return map; }
    GameObject* FindNearestGameObject(uint32 entry, float range) { assert(entry == 21117 && range == 60); return nearbyDoor ? &door : nullptr; }
    Creature* FindNearestCreature(uint32 entry, float range, bool alive) { assert(entry == 4832 && range == 120 && alive); return kelrisAlive ? &kelris : nullptr; }
};
namespace DcSettings
{
    bool enabled = false;
    bool GetBool(ObjectGuid, char const* key) { assert(std::string(key) == "EnableBlackfathomFireEvent"); return enabled; }
}
struct Step { std::string kind; uint32 entry = 0, state = 0; float radius = 0; uint32 timeout = 0; };
struct DungeonEvent
{
    uint32 map, id; bool persistent = false; uint32 panelBoss = 0;
    uint32 completeGo = 0, completeState = 0;
    std::function<bool(Player*, AiObjectContext*)> condition;
    std::vector<Step> steps;
};
struct EventBuilder
{
    DungeonEvent event;
    EventBuilder(uint32 map, uint32 id, char const*) : event{map,id,false,0,0,0,{}, {}} {}
    EventBuilder& Conditional(std::function<bool(Player*, AiObjectContext*)> f) { event.condition = f; return *this; }
    EventBuilder& Persistent() { event.persistent = true; return *this; }
    EventBuilder& PanelAfterBoss(uint32 entry) { event.panelBoss = entry; return *this; }
    EventBuilder& CompleteWhenGOState(uint32 entry, uint32 state, float) { event.completeGo = entry; event.completeState = state; return *this; }
    EventBuilder& MoveTo(float,float,float,float radius) { event.steps.push_back({"move",0,0,radius}); return *this; }
    EventBuilder& ClearRadius(float,float,float,float radius,float) { event.steps.push_back({"clear",0,0,radius}); return *this; }
    EventBuilder& Timeout(uint32 ms) { event.steps.back().timeout = ms; return *this; }
    EventBuilder& UseGO(uint32 entry,float radius) { event.steps.push_back({"use",entry,0,radius}); return *this; }
    EventBuilder& WaitForGOState(uint32 entry,uint32 state,uint32 timeout,float radius) { event.steps.push_back({"state",entry,state,radius,timeout}); return *this; }
    EventBuilder& KillCreatureEngage(uint32 entry,uint32,float radius) { event.steps.push_back({"kill",entry,0,radius}); return *this; }
    DungeonEvent Build() { return event; }
};
