"""Execute the actual active-escort threat filter with charmed ally/CC regressions."""
from pathlib import Path
import subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
s=(ROOT/'src/Ai/Dungeon/DungeonClear/Action/DcEngageActions.cpp').read_text()
a=s.index('        class ActiveEscortTarget');b=s.index('        auto allowed',a)
source=s[a:b]
doubles=r'''
#include <cassert>
#include <vector>
using ObjectGuid=unsigned;
struct Group { std::vector<unsigned> members; bool IsMember(unsigned id) { for(auto v:members) if(v==id) return true; return false; } };
struct Player;
struct Unit {
    unsigned guid=1; bool cc=false,assigned=false; Player* player=nullptr; Player* owner=nullptr;
    bool HasBreakableByDamageCrowdControlAura() { return cc; }
    bool IsPlayer() { return player; } Player* ToPlayer() { return player; }
    Player* GetCharmerOrOwnerPlayerOrPlayerItself() { return owner; }
    unsigned GetGUID() { return guid; }
};
struct Player:Unit { Group* group=nullptr; Player() { player=this; } Group* GetGroup() { return group; } };
struct PlayerbotAI { Player* bot; Player* GetBot() { return bot; } };
struct ThreatManager {};
struct FindNonCcTargetStrategy {
    PlayerbotAI* botAI; explicit FindNonCcTargetStrategy(PlayerbotAI* ai):botAI(ai) {}
    virtual void CheckAttacker(Unit*,ThreatManager*)=0;
    bool IsCcTarget(Unit* u) { return u->assigned; }
};
'''
scenarios=r'''
int main() {
    Player bot,ally,outsider; Group group; group.members={2}; bot.group=&group; ally.guid=2;
    PlayerbotAI ai{&bot}; ActiveEscortTarget selector(&ai); Unit hostile,pet;
    assert(selector.Allowed(&hostile));
    assert(!selector.Allowed(&bot)); assert(!selector.Allowed(&ally));
    pet.owner=&ally; assert(!selector.Allowed(&pet));
    pet.owner=&bot; assert(!selector.Allowed(&pet));
    hostile.cc=true; assert(!selector.Allowed(&hostile)); hostile.cc=false;
    hostile.assigned=true; assert(!selector.Allowed(&hostile)); hostile.assigned=false;
    assert(selector.Allowed(&hostile)); assert(!selector.Allowed(nullptr));
}
'''
with tempfile.TemporaryDirectory() as directory:
    p=Path(directory);(p/'test.cpp').write_text(doubles+source+scenarios)
    subprocess.run(['clang++','-std=c++20','-Wall','-Wextra','-Werror',str(p/'test.cpp'),'-o',str(p/'test')],check=True)
    subprocess.run([str(p/'test')],check=True)
# Both direct attackers and fallback selection must use the filter.
pick=s.split('Unit* PickEscortThreat',1)[1].split('// Distance-to-escortee',1)[0]
assert '!allowed(att)' in pick and 'return allowed(target) ? target : nullptr' in pick
print('RFK active escort charmed-party/pet and assigned/breakable CC scenarios passed')
