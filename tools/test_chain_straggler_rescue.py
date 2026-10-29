#!/usr/bin/env python3
"""Execute actual chain rescue/finalization helpers with controlled party doubles."""
import argparse
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--coverage-dir', type=Path)
args = parser.parse_args()
executor = (ROOT / 'src/Ai/Dungeon/DungeonClear/Util/DungeonEventExecutor.cpp').read_text()
chains = (ROOT / 'src/Ai/Dungeon/DungeonClear/Util/DcPartyWideChain.cpp').read_text()

def function(source, signature):
    start = source.index(signature)
    end = source.index('{', start) + 1
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

pull = function(executor, '    void PullStrandedFollowersAcross(')
followers = function(chains, '    std::vector<Player*> FollowerBots(')
finish = function(chains, 'void DcPartyWideChain::FinishFollowerChains(')
yield_start = chains.index('case StepOutcome::Stalled:', chains.index('bool DcPartyWideChain::DriveFollower('))
yield_end = chains.index('\n    }\n    return false;', yield_start)
yield_body = chains[yield_start + len('case StepOutcome::Stalled:'):yield_end]
# Test the old implementation too: omission of the event parameter still
# compiles, and the close-but-unfinished regression must fail at runtime.
chain_argument = ', &ev' if 'DungeonEvent const*' in pull else ''
prelude = r'''
#include <cassert>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>
using uint32=std::uint32_t;
struct DungeonEvent { uint32 id=6, mapId=48; };
struct DcChainState {
    uint32 eventId=0,mapId=0,instanceId=0,startedMs=0,stepIndex=0;
    DungeonEvent const* ev=nullptr;
    bool done=false,needsRescue=false;
    void Reset() { *this=DcChainState{}; }
};
struct MotionMaster { int clears=0; void Clear() { ++clears; } };
struct Player;
struct GroupReference {
    Player* p=nullptr; GroupReference* following=nullptr;
    Player* GetSource() { return p; }
    GroupReference* next() { return following; }
};
struct Group { GroupReference* first=nullptr; GroupReference* GetFirstMember() { return first; } };
struct Player {
    bool world=true,alive=true,ai=true,rider=false;
    uint32 map=48,instance=1;
    float x=0,y=0,z=0;
    int teleports=0;
    Group* group=nullptr;
    DcChainState state;
    MotionMaster motion;
    Group* GetGroup() { return group; }
    bool IsInWorld() { return world; }
    bool IsAlive() { return alive; }
    uint32 GetMapId() { return map; }
    uint32 GetInstanceId() { return instance; }
    bool GetVehicle() { return rider; }
    float GetOrientation() { return 0; }
    float GetExactDist(float a,float b,float c) { return std::sqrt((x-a)*(x-a)+(y-b)*(y-b)+(z-c)*(z-c)); }
    MotionMaster* GetMotionMaster() { return &motion; }
    void NearTeleportTo(float a,float b,float c,float,bool,bool,bool pet) {
        assert(pet);++teleports;x=a;y=b;z=c;
    }
    std::string GetName() { return "test"; }
};
#define GET_PLAYERBOT_AI(p) ((p)->ai)
#define LOG_DEBUG(...) ((void)0)
#define LOG_INFO(...) ((void)0)
bool handledFall=false;
bool HandleAsFall(Player*,DungeonEvent const&,DcChainState&,uint32) { return handledFall; }
uint32 getMSTime() { return 1000; }
namespace DcPartyWideChain {
    DcChainState* StateOf(Player* p) { return p->ai ? &p->state : nullptr; }
    void FinishFollowerChains(Player*, DungeonEvent const&);
}
'''
cases = r'''
int main() {
    DungeonEvent ev;
    Player leader,member;
    GroupReference ref{&member};Group group{&ref};leader.group=&group;
    auto prepare=[&] {
        member=Player{};ref.p=&member;
        member.state.eventId=ev.id;member.state.mapId=ev.mapId;member.state.instanceId=1;
    };
    auto rescue=[&] {
        PULL_CALL;
        DcPartyWideChain::FinishFollowerChains(&leader,ev);
    };
    // Final take-off is7yd from the landing on a disconnected island. Distance
    // alone cannot prove arrival: unfinished progress must force rescue.
    prepare();member.x=7;rescue();assert(member.teleports==1);assert(member.state.done);
    assert(member.motion.clears==1);assert(member.state.startedMs==1000);
    // A raw-path stall sets done to yield its tick, not to prove arrival.
    // Execute the production Stalled case to preserve that provenance.
    prepare();member.x=7;assert(!YieldToRescue(&member,ev,member.state));
    assert(member.state.done&&member.state.needsRescue);rescue();
    assert(member.teleports==1&&member.state.done&&!member.state.needsRescue);
    prepare();handledFall=true;assert(YieldToRescue(&member,ev,member.state));
    assert(!member.state.done&&!member.state.needsRescue);handledFall=false;
    // Completed same-instance chain near the landing is never yanked back.
    prepare();member.x=7;member.state.done=true;rescue();assert(member.teleports==0);
    // Unknown, stale-event, stale-map and stale-instance states must be rescued.
    prepare();member.x=7;member.state.Reset();rescue();assert(member.teleports==1&&member.state.done);
    prepare();member.x=7;++member.state.eventId;member.state.done=true;rescue();assert(member.teleports==1&&member.state.eventId==ev.id);
    prepare();member.x=7;++member.state.mapId;member.state.done=true;rescue();assert(member.teleports==1&&member.state.mapId==ev.mapId);
    prepare();member.x=7;++member.state.instanceId;member.state.done=true;rescue();assert(member.teleports==1&&member.state.instanceId==1);
    // Distant followers are still pulled even if their old chain reads done.
    prepare();member.x=100;member.state.done=true;rescue();assert(member.teleports==1);
    // Ineligible actors must neither be relocated nor falsely finalized.
    prepare();member.ai=false;rescue();assert(member.teleports==0&&!member.state.done);
    prepare();member.rider=true;rescue();assert(member.teleports==0&&!member.state.done);
    prepare();member.alive=false;rescue();assert(member.teleports==0&&!member.state.done);
    prepare();member.world=false;rescue();assert(member.teleports==0&&!member.state.done);
    prepare();member.map=1;rescue();assert(member.teleports==0&&!member.state.done);
    prepare();member.instance=2;rescue();assert(member.teleports==0&&!member.state.done);
    prepare();ref.p=&leader;rescue();assert(member.teleports==0&&!member.state.done);
    prepare();ref.p=nullptr;rescue();assert(member.teleports==0&&!member.state.done);
    // Legacy leader-only jumps keep their existing distance rule.
    prepare();member.x=7;PullStrandedFollowersAcross(&leader,0,0,0);assert(member.teleports==0);
    member.x=100;PullStrandedFollowersAcross(&leader,0,0,0);assert(member.teleports==1);
    leader.group=nullptr;rescue();
}
'''.replace('PULL_CALL', f'PullStrandedFollowersAcross(&leader,0,0,0{chain_argument})')

def run(folder, coverage=False):
    source=Path(folder)/'rescue.cpp';binary=Path(folder)/'rescue'
    source.write_text(prelude+pull+followers+finish+'bool YieldToRescue(Player* bot,DungeonEvent const& ev,DcChainState& st) {'+yield_body+'}'+cases)
    flags=['-fprofile-instr-generate','-fcoverage-mapping'] if coverage else []
    subprocess.run(shlex.split(os.environ.get('CXX','clang++'))+['-std=c++17','-Wall','-Wextra','-Werror']+flags+[str(source),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
if args.coverage_dir:
    args.coverage_dir.mkdir(parents=True,exist_ok=True)
    run(args.coverage_dir,True)
else:
    with tempfile.TemporaryDirectory(prefix='dc-chain-rescue-') as tmp:
        run(tmp)
assert 'PullStrandedFollowersAcross(bot,lx,ly,lz,ev);' in ''.join(executor.split())
print('Actual chain rescue/finalization regressions passed')
