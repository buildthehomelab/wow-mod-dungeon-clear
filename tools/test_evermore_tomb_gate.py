#!/usr/bin/env python3
"""Execute the actual TombMuster source against small world/settings doubles.

This is a compatibility-gate regression, not a core ABI or event executor test.
Run from any directory; requires a C++17 compiler (CXX, otherwise clang++).
"""
import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--coverage-dir', type=Path, help='retain LLVM-instrumented test source and binary')
args = parser.parse_args()
SOURCE = ROOT / "src/Ai/Dungeon/DungeonClear/Data/Events/BlackrockDepthsEvents.cpp"
text = SOURCE.read_text()
start = text.index("    ObjectiveArriveResult TombMuster(")
brace = text.index("{", start)
depth = 1
end = brace + 1
while depth:
    depth += (text[end] == "{") - (text[end] == "}")
    end += 1
function = text[start:end]

# The gate must precede the only automatic challenge in this event. Keeping
# the objective/door registration prevents a disabled gate silently skipping it.
event = text[text.index('EventBuilder(MAP_ID, EVENT_TOMB_OF_SEVEN,'):]
assert event.index('.Custom(HOOK_TOMB_MUSTER)') < event.index('.Gossip(NPC_DOOMREL,')
assert 'MakeObjective(OBJ(DcBlackrockDepths::OBJ_TOMB_OF_SEVEN)' in text
settings = (ROOT / 'src/Ai/Dungeon/DungeonClear/Settings/DcSettingsRegistry.h').read_text()
assert re.search(r'\{ "EnableBRDTombOfSeven",\s*DcType::Bool,\s*0,\s*0,\s*1,\s*false\s*\}', settings)
assert 'DungeonClear.EnableBRDTombOfSeven = 0' in (ROOT / 'conf/mod_dungeon_clear.conf.dist').read_text()
doors = (ROOT / 'src/Ai/Dungeon/DungeonClear/Data/DcEventDoorRegistry.h').read_text()
assert 'case 170576:' in doors and 'case 170577:' in doors

prelude = r'''
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>
using uint32 = std::uint32_t;
enum class ObjectiveArriveResult { Running, Done, Blocked };
struct AiObjectContext {};
struct DungeonBossInfo {};
constexpr uint32 TYPE_TOMB_OF_SEVEN=4, STATE_NOT_STARTED=0, STATE_IN_PROGRESS=1, STATE_DONE=3;
constexpr float TOMB_HOLD_X=0, TOMB_HOLD_Y=0, TOMB_HOLD_Z=0, TOMB_MUSTER_RADIUS=20, TOMB_MUSTER_ZBAND=5;
struct Player;
struct GroupReference {
    Player* p;
    GroupReference* following=nullptr;
    Player* GetSource() { return p; }
    GroupReference* next() { return following; }
};
struct Group { GroupReference* first=nullptr; GroupReference* GetFirstMember() { return first; } };
struct Player {
    Group* group=nullptr;
    bool world=true, alive=true;
    uint32 map=230;
    float distance=0, z=0;
    Group* GetGroup() { return group; }
    bool IsInWorld() { return world; }
    bool IsAlive() { return alive; }
    uint32 GetMapId() { return map; }
    float GetExactDist2d(float,float) { return distance; }
    float GetPositionZ() { return z; }
    std::string GetName() { return "test"; }
};
struct InstanceScript { uint32 state=0; uint32 GetData(uint32) { return state; } } instance;
bool hasInstance=true, enabled=false, throttled=true;
int logs=0;
namespace DcTargeting { InstanceScript* GetInstanceScript(Player*) { return hasInstance ? &instance : nullptr; } }
namespace DcSettings { bool GetBool(Player*, char const* key) { assert(std::strcmp(key,"EnableBRDTombOfSeven")==0); return enabled; } }
namespace DcThrottle { constexpr int BrdEventLog=1; }
namespace DcRun {
    struct Run { bool Throttled(int, int) { return throttled; } } run;
    Run& Of(AiObjectContext*) { return run; }
}
#define LOG_ERROR(...) (++logs)
#define LOG_INFO(...) ((void)0)
'''
cases = r'''
int main() {
    Player bot, member;
    AiObjectContext ctx;
    DungeonBossInfo info;
    auto eval = [&] { return TombMuster(&bot, &ctx, info); };
    using R=ObjectiveArriveResult;
    hasInstance=false; assert(eval()==R::Running); hasInstance=true;
    for (uint32 state : {STATE_NOT_STARTED,STATE_IN_PROGRESS,2u,4u}) {
        instance.state=state; enabled=false; assert(eval()==R::Blocked);
    }
    instance.state=STATE_DONE; enabled=false; assert(eval()==R::Done);
    enabled=true; assert(eval()==R::Done);
    instance.state=STATE_IN_PROGRESS; assert(eval()==R::Done);
    instance.state=STATE_NOT_STARTED; assert(eval()==R::Done); // no group
    GroupReference ref{&member}; Group group{&ref}; bot.group=&group;
    member.distance=100; assert(eval()==R::Running);
    throttled=false; assert(eval()==R::Running); throttled=true;
    member.distance=0; member.z=10; assert(eval()==R::Running);
    member.z=0; assert(eval()==R::Done);
    member.distance=100; member.alive=false; assert(eval()==R::Done);
    member.alive=true; member.world=false; assert(eval()==R::Done);
    member.world=true; member.map=1; assert(eval()==R::Done);
    ref.p=nullptr; assert(eval()==R::Done);
    group.first=nullptr; assert(eval()==R::Done);
    enabled=false; assert(eval()==R::Blocked); // opt-in withdrawn
    instance.state=STATE_DONE; assert(eval()==R::Done);
    assert(logs==5);
}
'''
def run(tmp, coverage=False):
    cpp = Path(tmp) / 'gate.cpp'
    binary = Path(tmp) / 'gate'
    source_line = text[:start].count('\n') + 1
    cpp.write_text(prelude + f'\n#line {source_line} "{SOURCE}"\n' + function +
                   '\n#line 1 "gate-cases.cpp"\n' + cases)
    flags = ['-fprofile-instr-generate', '-fcoverage-mapping'] if coverage else []
    subprocess.run(shlex.split(os.environ.get('CXX', 'clang++')) +
                   ['-std=c++17', '-Wall', '-Wextra', '-Werror'] + flags +
                   [str(cpp), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
if args.coverage_dir:
    args.coverage_dir.mkdir(parents=True, exist_ok=True)
    run(args.coverage_dir, coverage=True)
else:
    with tempfile.TemporaryDirectory(prefix='dc-tomb-gate-') as tmp:
        run(tmp)
print('Tomb gate: actual source execution and registration/settings checks passed')
