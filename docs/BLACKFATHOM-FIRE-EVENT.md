# Blackfathom fire event

October23,2026. Default-off DungeonClear.EnableBlackfathomFireEvent gates one
persistent conditional map48 event. It starts near the closed portal only after
Kelris is no longer alive nearby;the loaded door/room bounds the prerequisite.
The event clears the room,then uses fires21118,21119,21120,21121 in order.
Each use is followed by active-state confirmation and KillCreatureEngage for
4825,4977,4823,4978 respectively. That gate waits until *all* matching live actors
are gone,not merely count1. The actual server opens portal21117 after four DONE
fire states and all14 summoned deaths;the event confirms this real open state.
The door is script-only even with automation disabled;generic click bypass is
prevented. UseGO's state check skips already-active fires;Persistent keeps the
current step across combat gaps. A resumed run clears extant matching waves.
The event grants no item,force-opens no door,and edits no instance data.

Evidence:authenticated core48500d15204fc758894417ff1c06557babe35ad0 and
IP188f697a79a8d129816fd892bd44df962228d676;readonly map48 world inventory and
14 GO summon-group rows. Playerbots companion coverage is docs/BLACKFATHOM-MECHANICS.md
in the associated Playerbots PR. DC base1f1632d640ba9264798b43939b4bba8f4adee0da
is ahead of authenticated server pin c3722424ebe3bdf1d772a62c8ec7d3e3a3368bcd.
References:core Blackfathom instance header/script and SmartScript.cpp GossipHello
filter1:ordinary UseGO delivers the correct callback;ReportUse would be rejected.

Run python3 tests/test_blackfathom_event.py and tools/check_config_reads.py.
Source execution tests inspect actual table step order,configuration/map/door/
Kelris gates,persistence,all four wave mappings and real-door completion gate.
They use small engine/builder doubles and do not establish core ABI,real movement,
actual Use()/combat timing or client route completion. No full build,pmon,loaded
binary readback or client run. Independent and final-revision formal PR reviews
are mandatory. Keep unmerged,default-off and undeployed.

Acceptance:representative party kills Kelris,uses and clears each wave before
next fire,and reaches Aku'mai only after real portal opens. Test partial manual
fire completion,rejoined/resumed run,combat interruptions,wipe,reload,missing/
unreachable/despawned add,boss still alive,config disabled and reload/map exit.
Use failures/timeouts must leave a visible stalled event rather than bypassing
it. Measure pmon before/after. Missing waves cannot be recreated safely by blindly
reclicking active fires;human recovery may be required.

Existing limitation:current Aku'mai death SmartAI writes instance data5 but
current core TYPE_AKU_MAI is6;the core saves only slots0–5. This can leave the
post-boss altar locked. This event does not repair that source/data inconsistency
or claim complete altar/persistence validation. After an explicitly authorized
merge,prepare a separately reviewed wow-server pin PR for the actual reachable
resulting commit. No merge/deploy/enable/restart is authorized here.
