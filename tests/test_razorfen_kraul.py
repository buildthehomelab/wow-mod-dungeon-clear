"""Execute the actual ownership predicate and event table; no server build."""
from pathlib import Path
import subprocess
import tempfile
ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT / 'src/Ai/Dungeon/DungeonClear'
DOUBLES = r'''
#include <cassert>
#include <vector>
#include <string>
#include <functional>
#include <utility>
using uint32 = unsigned;
constexpr int QUEST_STATUS_INCOMPLETE = 1, SMART_ESCORT_TARGETS = 0;
struct Group {};
struct Player;
struct WorldObject { virtual ~WorldObject() = default; virtual Player* ToPlayer() { return nullptr; } };
using ObjectVector = std::vector<WorldObject*>;
struct SmartScript { ObjectVector owners; bool missing = false; ObjectVector const* GetStoredTargetVector(int, WorldObject&) { return missing ? nullptr : &owners; } };
struct AI { virtual ~AI() = default; };
struct SmartAI : AI { bool active = true; unsigned mEscortQuestID = 1144; SmartScript script; bool IsEscorted() { return active; } SmartScript* GetScript() { return &script; } };
struct Creature : WorldObject { SmartAI ai; unsigned map = 47; bool alive = true; unsigned GetMap() { return map; } AI* AI() { return &ai; } };
struct Player : WorldObject {
    unsigned map = 47; bool alive = true, near = true; int quest = QUEST_STATUS_INCOMPLETE;
    Group* group = nullptr; Creature* npc = nullptr;
    Player* ToPlayer() override { return this; }
    unsigned GetMapId() { return map; } unsigned GetMap() { return map; }
    bool IsAlive() { return alive; } Group* GetGroup() { return group; }
    int GetQuestStatus(int) { return quest; }
    bool IsWithinDistInMap(Creature*, float) { return near; }
    Creature* FindNearestCreature(int, float, bool) { return npc && npc->alive ? npc : nullptr; }
};
namespace DcSettings { bool enabled = false; bool GetBool(Player*, char const*) { return enabled; } }
struct AiObjectContext {};
using EventCondition = std::function<bool(Player*, AiObjectContext*)>;
struct Step { EventCondition escortActive; unsigned entry = 0; int gossip = 0; };
struct DungeonEvent { std::vector<Step> steps; EventCondition condition; unsigned map = 0; bool repeat = false, persistent = false, ownsMovement = false; };
struct EventBuilder {
    DungeonEvent event;
    EventBuilder(unsigned map, int, char const*) { event.map = map; }
    EventBuilder& Conditional(EventCondition c) { event.condition = c; return *this; }
    EventBuilder& Repeatable() { event.repeat = true; return *this; }
    EventBuilder& Persistent() { event.persistent = true; return *this; }
    EventBuilder& StepsOwnMovement() { event.ownsMovement = true; return *this; }
    EventBuilder& EscortCreature(unsigned entry, int gossip, unsigned, int, float, float, float, float) { event.steps.push_back({{},entry,gossip}); return *this; }
    DungeonEvent Build() { return event; }
};
'''
SCENARIOS = r'''
int main() {
    Player bot, owner, stranger; Creature willix; Group party, other;
    bot.group = owner.group = &party; stranger.group = &other;
    bot.npc = &willix; willix.ai.script.owners = {&owner};
    using DcRazorfenKraul::OwnedEscort;
    assert(!OwnedEscort(&bot)); DcSettings::enabled = true;
    assert(OwnedEscort(&bot) == &willix);
    willix.ai.script.owners = {&stranger}; assert(!OwnedEscort(&bot));
    willix.ai.script.owners = {&owner}; owner.quest = 2; assert(!OwnedEscort(&bot)); owner.quest = 1;
    owner.near = false; assert(!OwnedEscort(&bot)); owner.near = true;
    owner.alive = false; assert(!OwnedEscort(&bot)); owner.alive = true;
    owner.map = 0; assert(!OwnedEscort(&bot)); owner.map = 47;
    willix.ai.active = false; assert(!OwnedEscort(&bot)); willix.ai.active = true;
    willix.ai.mEscortQuestID = 1; assert(!OwnedEscort(&bot)); willix.ai.mEscortQuestID = 1144;
    willix.ai.script.missing = true; assert(!OwnedEscort(&bot)); willix.ai.script.missing = false;
    willix.alive = false; assert(!OwnedEscort(&bot)); willix.alive = true;
    bot.map = 0; assert(!OwnedEscort(&bot)); bot.map = 47;
    assert(OwnedEscort(&bot)); bot.alive = false; assert(!OwnedEscort(&bot)); bot.alive = true;
    std::vector<DungeonEvent> events; RegisterRazorfenKraulEvents(events);
    assert(events.size() == 1); auto& e = events[0];
    assert(e.map == 47 && e.repeat && e.persistent && e.ownsMovement);
    assert(e.steps.size() == 1 && e.steps[0].gossip == -1 && e.steps[0].entry == 4508);
    assert(e.condition(&bot,nullptr) && e.steps[0].escortActive(&bot,nullptr));
    owner.quest = 2; assert(!e.steps[0].escortActive(&bot,nullptr));
    DcSettings::enabled = false; assert(!e.condition(&bot,nullptr));
}
'''
def main():
    text = (BASE/'Data/Events/RazorfenKraulEvents.cpp').read_text()
    source = '\n'.join(line for line in text.splitlines() if not line.startswith('#include'))
    with tempfile.TemporaryDirectory() as directory:
        p = Path(directory); (p/'test.cpp').write_text(DOUBLES+source+SCENARIOS)
        subprocess.run(['clang++','-std=c++20','-Wall','-Wextra','-Werror',str(p/'test.cpp'),'-o',str(p/'test')],check=True)
        subprocess.run([str(p/'test')],check=True)
    driver=(BASE/'Action/DcEngageActions.cpp').read_text()
    escort=driver.split('bool DungeonClearEngageActionBase::DriveEscortCreature',1)[1].split('bool DungeonClearEngageActionBase::DriveDropInHole',1)[0]
    assert escort.index('!step.escortActive(bot, context)') < escort.index('ApplyEscortPartyRunSpeed')
    assert 'if (!step.escortActive && escortee->GetFaction()' in escort
    assert 'if (!step.escortActive)\n        ApplyEscortPartyRunSpeed' in escort
    conditional=driver.split('bool DcRunEventAction::Execute',1)[1]
    assert 'step.kind == EventStepKind::EscortCreature && step.escortActive' in conditional
    assert 'DriveEscortCreature(step, prog)' in conditional
    assert 'RegisterRazorfenKraulEvents(t)' in (BASE/'Data/DungeonEventRegistry.cpp').read_text()
    signal=(BASE/'Util/DcLeaderSignal.cpp').read_text()
    drop=signal.split('bool DcLeaderSignal::IsLeaderDroppingInHole',1)[1].split('Creature* DcLeaderSignal::GetLeaderEscortee',1)[0]
    assert 'OwnedEscort' not in drop
    assert 'DcRazorfenKraul::OwnedEscort(leader)' in signal.split('Creature* DcLeaderSignal::GetLeaderEscortee',1)[1]
    branch=conditional.split('step.kind == EventStepKind::EscortCreature && step.escortActive',1)[1].split('else if',1)[0]
    assert branch.index('DungeonEventExecutor::Drive') < branch.index('DriveEscortCreature')
    assert 'DungeonClear.EnableRazorfenKraulEscort = 0' in (ROOT/'conf/mod_dungeon_clear.conf.dist').read_text()
    print('RFK actual predicate/event ownership scenarios and integration checks passed')
if __name__ == '__main__': main()
