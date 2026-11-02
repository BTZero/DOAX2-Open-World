# Niki Beach terrain and actor research checkpoint

Latest follow-up: [SCENE_OWNERSHIP.md](SCENE_OWNERSHIP.md) traces native stage binding, no-hit fallback, and same-process volleyball/relaxation lifecycle. The original Niki relaxation scene exercised one observed native rig slot with this terrain trial inactive and returned normally; independent actor spawning remains unverified.

Date: 2026-11-01 local / 2026-11-02 UTC. Branch `feature/open-world`; baseline `23026d262d7b215d30ddfa1d263fef0c3696d945`, uncommitted research work. This updates [RESEARCH_FINDINGS.md](RESEARCH_FINDINGS.md). The previous report's statement that no gameplay experiment exists is superseded by the separately gated terrain trial described here. Free roam is still not implemented.

## Confirmed findings

The character fiber `sub_82511BB8` calls `sub_8250D530`, `sub_82510130`, `sub_8250F890` and `sub_82511F00` in its update loop, except when its native suppression flag is set. `sub_82510130` checks each of four slots through `sub_82511FA8`, applies native root motion through `sub_82510218`, then runs `sub_82513158` for active slots. The predicate checks native flags and motion/resource conditions; it is more meaningful than guessing occupancy from coordinates.

`sub_82513158` stores player position into a 412-byte rig record at `0x8342CA30 + slot*412`, calls the stage height method with X/Z and radius, and stores returned height at rig +44. It then invokes rig/animation services including `sub_82513930`. Facing is player-record +24, in the native angle representation; observed opposite court directions were 16384 and 49152. Native motion at player +52 is distinct from volleyball's high-level motion metadata. Observed IDs are leads, not portable walk/run bindings.

The stage object is `0x82E2DB50`, vtable `0x823BFD30`. Slot +44 points to `sub_825BEE88`, the height query. Its f1/f2 inputs are X/Z, f3 is radius, and r6 controls bypassing an optional override. With r6 zero, a predicate at object +112 can select a height callback at +116. Otherwise it uses general triangle-query services and a fallback height. `sub_825BAF98` tests X/Z bounds; `sub_825BAE30` and `sub_825BAEE8` locate spatial-cell triangle lists. This is a terrain system, not merely a visible mesh.

Volleyball startup `sub_82687798` installs predicate `sub_8268AE80` and height callback `sub_827609B8`. The predicate selects the override when the native activity is active and selector `0x833B7454` is zero. The callback returns zero. The original-mode live trace confirmed installed callback addresses and zero height results. This explains why court positions alone were inadequate terrain evidence.

## Captured geometry

The Niki scene has populated native terrain buffers:

| Object field | Meaning from generated code |
|---|---|
| +20 / +40 | Vertex pointer / count; 16-byte records, XYZ in first 12 bytes |
| +24 / +44 | Triangle pointer / count; 8-byte records, three big-endian 16-bit indices and a raw flag word |
| +28 / +32 | Spatial grid / triangle-list storage |
| +48,+56 / +64,+72 | X/Z bounds used by native bounds predicate |
| +80 | Spatial cell scale |
| +84 | Fallback height |
| +88 / +92 | Spatial grid dimensions |

Validated local capture `terrain-16600-20261102-010922-054.json`, screen 11, vacation table, activity selector 0: **453 vertices, 784 triangles**, all finite XYZ values and all triangle indices within 0..452. Vertex bounds were approximately X [-35.74,70.61], Y [-2.06,1.11], Z [-75.92,54.97]. Raw triangle flag values were 1,2,3,5,129,130,131,133; meanings remain unknown.

Offline geometric projection produced the following heights. This is a mathematical triangle intersection, **not** the native radius/material/walkability algorithm:

| X | Z | Projected Y | Raw flag |
|---|---|---|---|
| -1 | -5.4 | 0.02695 | 1 |
| 0 | 0 | 0.03539 | 1 |
| 0 | 10 | 0.07255 | 1 |
| 0 | 20 | -0.71014 | 1 |
| 10 | 0 | 0.59441 | 1 |
| 20 | 20 | -0.44043 | 129 |

There is non-flat terrain data beyond the court. This does not establish safe traversability, obstacle collision, swimming boundaries or valid player spawns throughout those bounds.

## Tooling and correction

The snapshot helper gained `-TerrainGeometry`, bounded read-only capture of those two buffers. Generated Windows accessors add `0x1000` for guest addresses at or above `0xE0000000`. Initial buffer captures omitted that adjustment and failed index validation. They are rejected evidence. Schema 2 captures verify the generated mapping and apply the adjustment; `scripts/analyze-research-terrain.py` rejects schema 1, malformed lengths, nonfinite vertices and invalid indices. Raw snapshots remain ignored copyrighted derived data and must not be published.

The probe now observes terrain binds, native height queries (separate sampling for the rig caller), and per-slot rig height, facing, motion and resource identity. An initial link failed because an existing macro already owned the rig hook. That was corrected by extending the existing wrapper, preserving its native60 bisect behavior. Subsequent probe builds compiled and linked successfully.

## Default-off native-grounding trial

The diagnostics build now contains independent restart-required CVar `doax2_open_world_terrain_trial=false`. `run-open-world-research.ps1 -Mode TerrainTrial` explicitly enables it and tracing. Ordinary Baseline builds omit this module; Probe and ProbeDisabled explicitly leave the trial disabled.

The trial declines the volleyball flat-height override only while inside the verified main-stage height query and only if all checks match: screen ID 11, vacation table `0x82AACFD0`, active state 1, activity selector 0, expected predicate/height callbacks, and bounded nonzero geometry counts. A scoped thread-local guard restores previous state after the query. The original predicate and native height function still run exactly once. No extra guest queries or replacement heights are injected. No actor transforms, timers, saves, input or camera controls are changed by this trial.

**Validation status at this checkpoint:** trial build succeeded and launch succeeded; actual terrain-trial Niki behavior is still pending. The trial session reached the original hotel/night flow while resuming the research save. No claim of successful terrain-following is made. Observation-only Niki execution and offline schema-2 geometry validation succeeded. Current launch evidence: `terraintrial-20261102-011331-195.log`, PID 5808. The physical-address read correction in probe source was made after this launch and still needs a final rebuild; the running executable already contains the terrain trial.

## Remaining work

### Resume audit, 2026-11-02 UTC

The later local run `terraintrial-20261102-012903-942.log` supersedes the pending-query status above. Its vacation-table screen 11 reached active state at update 2581. At update 2640, the rig caller `0x82513210` queried X=-1.5193, Z=-3.5103, radius=0.1, bypass=0 with the trial active and returned Y=-0.0050. Slot 0's subsequent rig sample recorded ground_y=-0.0050. Other rig slots recorded 0.0456, 0.0506 and 0.0441. Later player-record samples also contained nonzero Y values, including slot 0 Y=0.0274 near X=-0.9877, Z=-5.4062. This confirms execution of native non-flat grounding in the existing activity; it does not prove free-roam movement or visual stability.

That run ended with `Window closing`, `Execution complete` and normal title termination. Its last screen trace remained active screen 11: closing the application is **not** evidence of activity cleanup, return to location selection, or repeatable re-entry. Those checks remain outstanding.

The latest Probe build was configured and built successfully during the resume audit, including the physical-address read correction. Fresh TerrainTrial run `terraintrial-20261102-033037-702.log` (PID 16908) resumed the isolated research save at Day 3 Daytime and entered Niki volleyball. Screen 11 reached active state at update 3651. The activity rendered four animated characters and accepted a 1.5-second W movement input. At update 4170 the rig query at X=-1.3028, Z=1.8875 returned 0.0246 and slot 0's rig ground_y was also 0.0246; the other slots reported 0.0226, 0.0637 and 0.0473. The captured court views showed characters on the beach; they do not establish foot-contact quality throughout all animation frames.

Normal pause-menu Give Up / Yes transitioned screen 11 to exit state at update 4946, ran the original calendar call from `0x826CF994` at update 4955 (9 to 10), and returned to active location selection, screen 1 / table `0x82AACFD0`, at update 5020. Thus one fresh entry/movement/normal-exit cycle passed without an observed crash. Calendar advancement is expected original activity behavior, not unlimited exploration. Repeated re-entry, detailed rig stability, disabled activity comparison and movement outside the court remain untested. The session was then closed using the normal window close helper. All trial behavior remains default-off.

1. Finish the native-grounding trial at Niki. Compare normal zero-height logs with native mesh heights; inspect character rig stability, court movement, normal exit and re-entry. If behavior regresses, keep the trial off and diagnose the native query policy.
2. Trace terrain binding caller and resource loading/unloading. Confirm material flags, supported slopes, no-hit behavior and obstacle/contact handling before moving outside court bounds.
3. Map actor spawn/assignment, compatible idle/walk/run clips and camera ownership. Keep root-motion application single-owned.
4. Establish a mode-local stationary single actor with required services and normal cleanup. Only then add third-person camera, grounded movement and suppression of competition/calendar progression.

This advances T03/T04 research and supplies a concrete reversible experiment. It does not complete the single-actor milestone or Phase 1 acceptance.
