# v0.2.4 Combat Feel and Balance Notes

Date: 2026-07-22
Status: Final v0.2.4 evidence captured; awaiting the separately required user approval for the single implementation commit.

## Environment and authority

- Project: `ProjectR`, UE 5.8, Development Editor.
- Runtime authority remains the existing Player/Enemy/Boss DataAssets and GameplayEffects. This document is evidence only; it is not a runtime balance source.
- Snapshot evidence: `Saved/BalanceReports/v024/authoritative-snapshot.json`.
- Objective death-risk evidence: `Saved/BalanceReports/v024/player-death-risk-20260721T223956.json`.
- Required candidate assets verified: 40. Three existing Enemy Prototype packages were precisely saved in the approved first tuning round. The separately approved BasicAttack compatibility revision created exactly two packages (`/Game/ProjectR/Abilities/BasicAttack/DA_BasicAttack` and `/Game/ProjectR/Abilities/BasicAttack/GA_BasicAttack`) and modified only `/Game/ProjectR/Abilities/DA_DefaultAbilitySet`; it is not a second balance authority.

## Baseline snapshot

| Area | Authoritative fields | Baseline | Final | Status |
| --- | --- | --- | --- | --- |
| Player | MaxHealth / MaxShield / MaxEnergy / AttackPower / MoveSpeed | 100 / 50 / 100 / 10 / 600 | Not frozen | Retained candidate |
| BasicAttack | 150 cm / 45 cm, 10 + 1.0 AP, 0.08 / 0.06 / 0.22 s, zero Energy/Cooldown | Absent | Approved contract revision; not a balance-tuning candidate | Changed |
| ShadowThrust | Base + AP / Energy / Cooldown / displacement | 25 + 1.5 AP / 20 / 4.0 s / 450 cm | Not frozen | Retained candidate |
| FireSlash | Base + AP / Energy / Cooldown | 15 + 1.0 AP / 25 / 5.0 s | Not frozen | Retained candidate |
| ThunderDrop | Base + AP / Energy / Cooldown | 25 + 2.0 AP / 30 / 7.0 s | Not frozen | Retained candidate |
| AfterimageDodge | Energy / Cooldown / displacement | 15 / 2.5 s / 300 cm | Not frozen | Retained candidate |
| VectorHook | Energy / Cooldown / range | 15 / 4.0 s / 800 cm | Not frozen | Retained candidate |
| CounterProofWall | Energy / Cooldown / duration | 20 / 6.0 s / 1.0 s | Not frozen | Retained candidate |
| MeleeMinion | Health / Shield / AP / speed | 100 / 0 / 10 / 450 | 50 / 0 / 10 / 450 | Changed; awaiting fresh human evidence |
| RangedMinion | Health / Shield / AP / speed | 80 / 0 / 10 / 380 | 40 / 0 / 10 / 380 | Changed; awaiting fresh human evidence |
| ShieldMinion | Health / Shield / AP / speed | 140 / 80 / 12 / 300 | 70 / 40 / 12 / 300 | Changed; awaiting fresh human evidence |
| EliteAuditGuard | Health / Shield / AP / speed | 300 / 150 / 15 / 350 | Not frozen | Retained candidate |
| Auditor | Health / Shield / AP / speed | 900 / 120 / 18 / 320 | Not frozen | Retained candidate |
| Auditor phase | RuleAudit / PredictionShield / shield value | 0.66 / 0.33 / 240 | Not frozen | Retained candidate |

## Automated evidence before human runs

- BuildEditor: PASS (`20260721T221742Z-fa352dd8` after the BasicAttack tool-timing repair).
- Input 3/3, GAS 4/4, Combat 5/5, Ability 6/6, Save 5/5, Debug 12/12 (`-game /Game/ProjectR/Maps/L_CombatGym`), PlayerSkill 5/5, Enemy 11/11, Boss 3/3, and `ProjectR.UI.CombatHUD` 6/6: fresh PASS in isolated process contexts.
- Fixed PlayerSkill D and E PIE smokes: PASS with seven formal AbilitySet specs; D covers VectorHook/CounterProofWall and E covers six-P0 presentation without giving BasicAttack a SkillBar slot.
- Balance tool schema and no-argument boundary: PASS 2/2 (`ProjectRAuthoringTools.Balance`).
- Player death risk: PASS in 15.001 seconds through the formal Shield -> Health -> Death CombatSubsystem chain; no direct attribute write or synthetic test damage.
- Auditor fixed smoke: PASS for P1/P2/P3, one completion fragment, and runtime cleanup.
- Two automatic no-input attempts of the human CombatGym instrumenter reached its 180-second safety cap. They are explicitly rejected as non-human, non-acceptance evidence.

## Approved tuning round 1: normal encounter readability and pacing

| Package | Field | Old | New | Unit | Rationale | Regression assertion | Rollback |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `/Game/ProjectR/Enemies/Prototypes/DA_Enemy_MeleeMinion` | `Attributes.Health`, `Attributes.MaxHealth` | 100, 100 | 50, 50 | health | User could not read ordinary enemy health and reported the fixed encounter as too durable. | `ProjectR.Enemy.Assets.CheckpointAManifest` | 100, 100 |
| `/Game/ProjectR/Enemies/Prototypes/DA_Enemy_RangedMinion` | `Attributes.Health`, `Attributes.MaxHealth` | 80, 80 | 40, 40 | health | Same approved normal-encounter adjustment; all attack and movement data retained. | `ProjectR.Enemy.Assets.CheckpointBManifest` | 80, 80 |
| `/Game/ProjectR/Enemies/Prototypes/DA_Enemy_ShieldMinion` | `Attributes.Health`, `Attributes.MaxHealth`, `Attributes.Shield`, `Attributes.MaxShield` | 140, 140, 80, 80 | 70, 70, 40, 40 | health / shield | Same approved normal-encounter adjustment while retaining Heavy Mobility and Shield spill-over mechanics. | `ProjectR.Enemy.Assets.CheckpointCManifest` | 140, 140, 80, 80 |

- RED: after changing only these formal asset assertions, the three manifests failed exactly on the old values; `CheckpointDManifest` passed. Evidence: `Saved/Logs/ProjectR-backup-2026.07.21-20.56.26.log`.
- MCP read-before-write verified exact original values, Tags, perception, and AttackDefinitions. The post-write and restart reads matched the table; only the three listed packages were dirty before their exact save.
- BuildEditor: PASS (`20260721T205455Z-c7145f11`).
- GREEN: `ProjectR.Enemy.Assets` 4/4 PASS; evidence: `Saved/Logs/ProjectR.log` at `2026.07.21-21.03.11`.
- This round remained non-final until the post-mutation CombatGym runs and the user's later BossGym acceptance were obtained; no further balance asset mutation superseded it.
- The user also approved a compatibility repair: Dead Enemy Actors remain available for future Revive but ignore Pawn collision until revived; this is not an HP/attack-cadence adjustment. BasicAttack and its two exact Packages were added after the first temporary normal-enemy health round. The five post-mutation CombatGym records and the user's later BossGym PASS conclusion are captured below.

## Final evidence gates

For v0.2.4 code-function validation, the user accepted the observed quick encounter tempo. Five CombatGym and three BossGym durations remain required evidence, but the former 60–120-second and 180–300-second ranges are post-v0.2.4 encounter-pacing observations rather than completion gates.

## Recorded CombatGym function-validation runs

The user completed five fixed CombatGym runs after the approved normal-enemy mutation and BasicAttack compatibility revision. Every recorded run reached the runner's runtime-clean postcondition. The user explicitly accepted the observed quick tempo for code-function validation and asked that encounter pacing be deferred to a later version.

| Run | Duration (s) | Health | Shield | Energy | Damage taken | Hit count | Unique skills | Runtime clean |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 1 | 31.195 | 72 | 0 | 0 | 78 | 5 | 4 | PASS |
| 2 | 16.225 | 100 | 4 | 55 | 46 | 3 | 3 | PASS |
| 3 | 14.934 | 90 | 0 | 30 | 60 | 4 | 4 | PASS |
| 4 | 38.397 | 100 | 4 | 55 | 46 | 4 | 3 | PASS |
| 5 | 9.672 | 89 | 0 | 55 | 61 | 4 | 3 | PASS |

- Median duration: **16.225 s**. It is an accepted observation for this function-validation pass, not a v0.2.4 encounter-pacing target.
- The fixed instrumenter did not emit per-enemy kill times, Energy-insufficient counts, Cooldown-rejection counts, or a standalone death field for these five records. Those fields are therefore not represented as PASS evidence.
## Recorded BossGym manual acceptance

- The user manually completed the remaining BossGym runbook and explicitly stated that **all checks passed**. This is the required subjective conclusion for P1 Sweep/LockShot/Shield readability, P2 rule behavior, P3 PredictionShield blocking and alternate-skill break, Boss defeat, one fragment, no profile/save deletion, and stable camera/input/X-Z plane.
- The manual instrumentation was not active during those user-controlled runs. Therefore per-run total time, P1/P2/P3 durations, resource values, damage taken, hit counts, skill distribution, and PredictionShield counts were not retrospectively invented. The raw human conclusion is authoritative for subjective acceptance; numeric timing ranges remain deferred encounter-pacing observations.
- Final objective evidence: `Saved/AutomationReports/v024-final-report-20260722/v024-final-None/result.json` reports **47/47 required PASS**. Optional `PhysicalGamepad`, `NetworkPIEReplication`, `DevelopmentPackage`, `ShippingPackage`, and `GC` remain `NOT_RUN`.

## Final classification and compatibility review

- Changed: MeleeMinion Health/MaxHealth 100 to 50; RangedMinion Health/MaxHealth 80 to 40; ShieldMinion Health/MaxHealth 140 to 70 and Shield/MaxShield 80 to 40; approved BasicAttack assets/AbilitySet entry/Tag; Dead Enemy Pawn-collision suppression; fixed Auditor spawn transform to avoid the BossGym PlayerStart overlap.
- Retained: Player default attributes, six P0 Skill values, Enemy attack data/cooldowns, Elite values, Auditor values/phases/PredictionShield, existing 0.10-second player HitReaction, all maps, HUD, Registry, StateTree, Enemy AbilitySet, GameplayEffects, and Save semantics.
- Not Applicable: new Balance DataAsset, Balance Subsystem, new enemy/Boss attack, input asset, map, HUD/UI asset, Director, QTE, Companion, Reward, Room, Profile, Save schema, analytics, or shipping behavior.
- v0.3.0/3.1 consume the final asset authority without resetting combat values or directly writing Enemy Health; v0.3.2 only subscribes to stable Ability/Combat/Enemy/Boss state; v0.4.0 Director applies whitelisted deterministic deltas rather than overwriting this baseline; v0.4.2/4.4 Meta/Room/Reward add controlled increments only; v0.7.x content does not replace Combat/Enemy/Boss contracts; v0.8.x accessibility/rebinding/difficulty preserve input semantics; v0.9.1/v1.0.1 can replay the fixed Gym checks and trace final values to this document.

## Known risks and downstream observation

- `HitReactionDurationSeconds` remains the existing runtime C++ value of 0.10 seconds. It is not a v0.2.4 asset tuning field; changing it requires a separate contract revision and dataization proposal.
- v0.3.0+ must consume this final baseline as one authority and must not introduce a parallel balance DataAsset or subsystem.
- Future Companion, QTE, Director, Room/Reward, and Shipping QA systems consume existing combat state/events and final asset values without direct runtime overwrites.
