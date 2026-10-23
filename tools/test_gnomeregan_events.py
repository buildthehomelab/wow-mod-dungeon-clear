"""Execute the Gnomeregan table authoring code with data-store/builder doubles.
This verifies authored data and fail-closed DBC lookup, not the escort executor.
"""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'src/Ai/Dungeon/DungeonClear/Data/Events/GnomereganEvents.cpp'


def main():
    code = r'''
#include <cassert>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>
using uint32 = uint32_t;
using int32 = int32_t;
struct DungeonEncounterEntry { uint32 id=379,mapId=90,encounterIndex=7; };
struct Store {
    DungeonEncounterEntry entry;
    bool present=true;
    DungeonEncounterEntry const* LookupEntry(uint32 id) { return present && id==379 ? &entry : nullptr; }
} sDungeonEncounterStore;
struct DungeonEvent {
    uint32 mapId=0,index=0,escort=0,doneEntry=1;
    int32 doneBit=-1;
    bool persistent=false;
};
struct EventBuilder {
    DungeonEvent row;
    EventBuilder(uint32 map, uint32, char const*) { row.mapId=map; }
    EventBuilder& Anchored(uint32 i) { row.index=i; return *this; }
    EventBuilder& Persistent() { row.persistent=true; return *this; }
    EventBuilder& MoveTo(float,float,float,float) { return *this; }
    EventBuilder& EscortCreature(uint32 entry,int32 option,uint32 done,int32 bit,float,float,float,float) {
        assert(option==0); row.escort=entry; row.doneEntry=done; row.doneBit=bit; return *this;
    }
    DungeonEvent Build() { return row; }
};
struct Anchor { uint32 entry,index,map,event; };
struct BossRosterPatch { uint32 mapId; std::vector<uint32> remove; std::vector<Anchor> add; };
namespace DcRoster {
uint32 OBJ(uint32 seq) { return 900000+seq; }
Anchor MakeObjective(uint32 entry,uint32 index,uint32 map,char const*,float,float,float,float,uint32,uint32,uint32 event) {
    return {entry,index,map,event};
}
}
'''
    code += '\n'.join(line for line in SOURCE.read_text().splitlines() if not line.startswith('#include'))
    code += r'''
int main() {
    std::vector<DungeonEvent> events;
    std::vector<BossRosterPatch> roster;
    RegisterGnomereganEvents(events); RegisterGnomereganRoster(roster);
    assert(events.size()==1 && roster.size()==1);
    assert(events[0].mapId==90 && events[0].persistent && events[0].escort==7998);
    assert(events[0].index==7 && events[0].doneBit==7 && events[0].doneEntry==0);
    assert(roster[0].remove==std::vector<uint32>{7361});
    assert(roster[0].add[0].index==7 && roster[0].add[0].event==1);
    for (int kind=0;kind<3;++kind) {
        events.clear(); roster.clear();
        sDungeonEncounterStore.present=kind!=0;
        sDungeonEncounterStore.entry.mapId=kind==1 ? 1 : 90;
        sDungeonEncounterStore.entry.encounterIndex=kind==2 ? 32 : 7;
        RegisterGnomereganEvents(events); RegisterGnomereganRoster(roster);
        assert(events.empty() && roster.empty());
    }
}
'''
    for path, registration in [
        ('src/Ai/Dungeon/DungeonClear/Data/DungeonEventRegistry.cpp', 'RegisterGnomereganEvents(t);'),
        ('src/Ai/Dungeon/DungeonClear/Overrides/BossRosterRegistry.cpp', 'RegisterGnomereganRoster(t);'),
    ]:
        assert registration in (ROOT / path).read_text()
    with tempfile.TemporaryDirectory() as temporary:
        folder = Path(temporary)
        (folder / 'test.cpp').write_text(code)
        subprocess.run(['clang++', '-std=c++20', '-Wall', '-Wextra', '-Werror', str(folder / 'test.cpp'), '-o', str(folder / 'test')], check=True)
        subprocess.run([str(folder / 'test')], check=True)
    print('Gnomeregan event authoring checks passed')


if __name__ == '__main__':
    main()
