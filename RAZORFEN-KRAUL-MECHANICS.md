# Razorfen Kraul mechanics coverage

Source audit: October 23, 2026. Target map47 only. Proposed, default-off and unmerged.
Source/data coverage does not establish loaded binaries, DBC revision or a successful client route.

## Evidence

Authenticated Evermore core48500d15204fc758894417ff1c06557babe35ad0,
Playerbots base7779c91e70bf7f26547126850cadf7e881e71faa,
Dungeon Clear base5d49ee7ab0595de4031c73f6a2b41e17b371d13c,
Individual Progression main188f697a79a8d129816fd892bd44df962228d676.
IP RFK file blob39a60f96bb7a0744a3df149976096d8e0e7eccd4 matches current main
and inspected master4933d455d2f611eed50c74c68c78832df4ed37a3. Current server manifest
pins core/IP/DC to these commits but Playerbots to22e730d480d5b29ece25ad7aafa0bbb58d46fe4d.
The newer Playerbots source is the PR base; it is not claimed deployed.

Readonly world evidence:191 fixed spawn rows/38 actor types;11 additional spell/quest
summon types;125 fixed actor SmartAI rows,11 summon rows and4 reachable actionlist478100
rows;68 object rows/31 types,6 object AI rows;47 Willix waypoints;6 RFK zone1717 quests.
Initial broad numeric-range query also fetched716 unrelated actionlist rows; those
are excluded. No pooled creature rows or selected summon-group rows were returned.
Actor and object CSVs enumerate every selected type, including rares, neutrals,
quest NPCs, resources, transport and decorative fire. C++ go_flames additionally
creates transient world triggers; these are spell emitters rather than attackable adds.

Raw snapshots live in workspace artifacts/razorfen-kraul-mechanics/source/. Base
SmartAI is selected from the fetched authenticated core. Live rows confirm progression
replacements and bindings. Spell effects come from saved Evermore z80 reference
Spell.dbc, not loaded-server DBC readback. No selected cast spell_dbc overrides or
selected spell_linked_spell rows were found. This is not an exhaustive migration replay.

Core source: [RFK instance](https://github.com/buildthehomelab/wow-azerothcore/blob/48500d15204fc758894417ff1c06557babe35ad0/src/server/scripts/Kalimdor/RazorfenKraul/instance_razorfen_kraul.cpp),
[SmartAI](https://github.com/buildthehomelab/wow-azerothcore/blob/48500d15204fc758894417ff1c06557babe35ad0/src/server/game/AI/SmartScripts/SmartAI.cpp),
[progression overrides](https://github.com/buildthehomelab/wow-mod-individual-progression/blob/188f697a79a8d129816fd892bd44df962228d676/data/sql/world/base/dungeon_razorfen_kraul.sql).

## Bosses, optional and quest actors

| Actor | Actual mechanics and response |
| --- | --- |
| Roogug6168 | Lightning Bolt9532: generic interrupt, tank and class damage. Quest loot retained. |
| Aggem4424 | Battle Shout6192, casted Boar Spirit8286→6021 and Chain Heal14900: generic interrupt/tank add acquisition. No fabricated boar floor. |
| Jargba4428 | Shadow Bolt9613, magic charm14515: generic interrupts and legal dispels plus opt-in shared Common/Charmed control, self/splash/channel suppression and target/pet cleanup. No new class spell selector. |
| Ramtusk4420 | Call for help10yd, Battle Shout9128 and instant Thunderclap15548. Pull deliberately; generic tank/group response, magic slow cure if eligible; no cast-time escape for instant hit. Quilguard Champion4623 is also a runner. |
| Agathelos4422 | Rampage8285 is instant area damage+stun; Left for Dead8555 is single-target damage; Enrage8269 is dispel-type0. Generic role/defensive/healing response. Do not invent tranquilize legality from spell name. Ward gate below controls release. |
| Charlga4421 | Chain Bolt8292, Renew6077, Purity8361 school immunity, Mana Spike8358. Generic interrupt, offensive magic purge for Renew, immunity-aware targeting and healing; no floor. |
| Blind Hunter4425 | AI-init70% despawn chance, Shock12553; optional presence/absence, generic tank/damage. Not a guaranteed spawn. |
| Earthcaller Halmgar4842 | AI-init70% despawn chance, Rumbler8270→4528, Lightning Bolt9532, Earthbind2484→2630 and assist flee. Generic interrupts, nearby runner/totem focus; no mandatory rare wait. |
| Willix4508 | Quest1144 acceptance starts SmartAI path4508/faction113. Ambush4514 at waypoints5/15/44;47 total points. Optional DC default-off protection of already-active owned escort. Completion uses owner's actual quest status, never final speech/coordinates. |
| Heralath4510 | Starts Mortality Wanes1142. Quest interaction stays quest-owned; not a hostile encounter. |
| Gopher4781 | Crate spell6918 summons it; stick6684 casts8283. Data exchange with tuber20920 invokes478100, uncovers/respawns tuber300s, then follows summoner. Optional quest chain, no forced GO use. |

## Trash, recursive summons and class ownership

The actor CSV is the exhaustive per-entry matrix; no fixed or summoned actor is
excluded merely for lacking SmartAI.22 verified assist-runner entries are eligible
only during real assistance/flee movement. Passive ASSISTANCE_MOTION is recognized;
fear fallback is accepted only with non-passive reaction. Melee stays within12yd;
ranged uses its normal spell range. Tanks/healers retain their acquisition behavior.

Quilguard/Defender/Warrior/Warden/Spearhide/Totemic, all Death's Head casters,
Geomancer/Dustweaver/Groundshaker/Earthbreaker, Handler/Trainer/Beastmaster,
Champion/Halmgar/Stalker and Ward Guardian are covered. Acolyte Renew/Mana Burn,
Priest Heal/Fortitude/Shadow Bolt, Adept Frostbolt/Chains, Guardian Fireball/Healing
Wave, Dustweaver Enveloping Winds and other caster spells retain generic interrupts,
magic/poison/curse cures, immunity and range/LOS/cooldown rules. Blood of Agamaggan's
Curse of Blood8282 is curse; Rotting Agam'ar's Cursed Blood8267 is not classified by
its name. Normal Agam'ar charge, Raging Agam'ar frenzy, bats' Shock and Greater
Bat's instant Sonic Burst8281 retain generic roles/healing. No fabricated cast escape
for instant Ground Tremor6524, Rampage8285, Thunderclap15548 or Sonic Burst8281.

Spearhide Whirling Barrage8259 has a real cast and caster-area weapon hit: opt-in
cast-only escape uses loaded SpellInfo effect radii, includes overlaps, applies3D
proximity gating to avoid remote floor casts, and ends when the cast ends. It is
not a persistent floor. Lower relevance than generic interrupts/dispels/emergencies.

Recursive summons: Earthbind2630→6474→3600 magic slow; Healing Ward2992→4972→5016
heal; Elemental Protection6016→8263 resistance aura; Lava Spout6017→8266→8265
periodic area damage; Earthgrab6066→8377 instant area root; Rumbler4528,
Howler4526, battleboar4535→6268, hyena4534 and Boar Spirit6021 use their actual
bound AI/template spells. Nearby DPS can destroy the five verified totem types
only when their living hostile summoner is actively targeting this party or its
controlled unit. Cached possible-target list is reused; idle/unrelated totems do
not initiate pulls. Existing generic tank/add and class damage remain responsible
for other summons. Killing Lava Spout stops future pulses; movement cannot undo
an applied root or past damage. No persistent lava floor is invented.

Shared charm behavior is map/config gated. Core faction flips and dispel legality
still apply. Shared contained-charm protection intentionally blocks stripping party
CC from a contained ally; it does not grant missing spells. Focus/hazard multipliers
exempt RcLeaveCharmedTargetAction, manual AttackMyTarget and duel acquisition.
Focus actions revalidate live CC/order/range/LOS and invalidate rejected focus cache.
Hooks register unconditionally so config reload can enable existing engines.

## Objects, gates and quests

Ward21099's5s SmartGO update is conditional on NO living Ward Keeper4625 within50yd;
only then does it open and send data to Agathelos. Keeper7083 channel and this
condition remain authoritative. No DC force-open or synthetic instance state.
The core RFK instance class has no encounter/save/door state machine.

All31 object types appear in the object CSV: chests, herbs/ore, elevators,
tuber/node, ward and go_flames campfires/cauldrons. Core go_flames checks0.3yd
contact each3s and creates a2s trigger casting7897; these are point-contact
ambient objects, not a large retained combat floor. Opt-in contact escape
uses the existing cached nearest-gameobject GUID list and verified fire entries,
with the core IsInRange3d model-bounds predicate expanded by1.3yd
(0.3yd contact plus1yd margin), including out of combat. Escape encloses the
loaded scaled/rotated display bounds and per-axis margin; missing display
data uses object-size fallback.
It combines with overlapping cast hazards and checks3D distance/spawn/map state;
it does not draw a large retained floor from the triggered spell radius.
Navigation/client acceptance must check that bots do not park on them.
Elevator/pathfinding success remains a runtime acceptance item.

RFK zone1717 quests1101/1102 (Charlga loot),1109 (bat guano),1142 (pendant),1144
(escort) and1221 (tubers) are included, plus external warrior1701 Roogug loot
context. Optional loot/interactions keep generic quest/loot systems; no quest is
accepted, shared, completed or rewarded by these changes. DC follows Willix only
with active SmartAI quest1144 and stored escort player in this party, alive/nearby,
on this map and INCOMPLETE. It never restarts his path, changes faction or pace,
adopts another party's escort, or blocks an ineligible party's route.

## Verification and acceptance

Local source execution tests cover focus, all runner IDs, passive assistance,
manual acquisition/duels, live explicit/skull validity, late CC, map/config reload,
totem ownership and idle rejection, cast overlaps, capped partial progress, roots,
failed paths/casts and deferred PointMovement launch, plus runner/hazard/charmed ally
cleanup overlap. DC executes the actual ownership predicate/event table and verifies
integration placement/lifecycle. Doubles do not prove ABI, collision or real casts.
No full build (repository policy), pmon or game-client route is claimed.

Client acceptance must traverse every boss/trash/object CSV entry, each rare
present/absent, all summon kinds, all Willix ambushes, Blueleaf chain, quest loot,
ward release and elevators. Test tank/healer/melee/ranged roles, pets, unavailable
interrupt/dispel classes, immunity, manual skull/target/CC changes, nearby unrelated
totems/escorts, party pet victims, death/evade/map/instance transitions, config reload,
charm+runner+Whirl overlaps, roots, blocked paths and stacked passages. Check campfire
contact, narrow bridges, recovery, loot and final route completion in the client.
Measure playerbot pmon before/after using100% active-alone bots and zero smart scale,
then five-minute stack samples with feature off and on. Record loaded binary/DBC
revisions independently from source/readonly data.

No merge, pin, DB/config write, enablement, deployment or restart is authorized.
After an authorized merge, resolve each actual reachable resulting commit and prepare
separate reviewed wow-server pin PRs; never pin the proposal or advance unrelated pins.
Other dungeon PRs are references only, not dependencies.
