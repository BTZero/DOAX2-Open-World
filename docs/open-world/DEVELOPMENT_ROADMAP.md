# Dependency-aware roadmap and Niki Beach implementation specification

Status: the user subsequently authorized autonomous preparation, experiments and game navigation. T01 and the observational portion of T02 have been executed; T03/T05 are partially mapped. No playable free-roam implementation exists. See [RESEARCH_FINDINGS.md](RESEARCH_FINDINGS.md) for current evidence, remaining validation and the next experiment. The dependency and acceptance plan below remains applicable.

## Delivery order and dependencies

Keep the requested phase order. Generated source is now available, but Niki Beach resource ownership and terrain contracts still require discovery gates inside Phase 1. No evidence supports moving world expansion or a GUI ahead of locomotion.

```text
Phase 0 documentation and user review
  -> baseline SDK/input/codegen/build and isolated original-mode run
  -> read-only Niki Beach scene/actor/camera/transition mapping
  -> terrain, animation, save-write and teardown contracts
  -> Phase 1 isolated environment + actor + bounded walk/run + camera + exit
      -> Phase 2 interactions and reusable animation catalog
      -> Phase 3 free camera / UI hiding / screenshots / optional posing
      -> Phase 4 standalone FreeCruise using lifecycle and configuration patterns
      -> Phase 5 clock + verified visible lighting changes
      -> Phase 6 vacation rules + optional economy/activities + persistence
      -> Phase 7 controlled location links, then investigate seamless expansion
```

Phase 3 basic free camera depends on Phase 1 camera/lifecycle, while posing benefits from Phase 2 animation work. Phase 4 technical research can be independent of interactions/photo mode; its delivery stays after them. Phase 5 clock design can be independent, but visible lighting requires environment bindings. Phase 6 persistence includes the Phase 5 clock; the format can be scoped earlier without enabling guest save writes. Phase 7 depends on safe scene ownership, collision and memory measurements, with vehicle integration requiring Phase 4.

Independent research after approval: asset metadata classification, function annotation, controller/schema design, Marine Race route audits and lighting inventory. Keep results as evidence and small experiments. Do not introduce several unverified gameplay systems together.

## Phase 1 engineering specification

### 1. Baseline and discovery

Prepare the SDK and supported local input using the existing scripts; generate guest code; build original mode before adding gameplay logic. Record XEX hash, SDK revision/patch, toolchain, renderer and timing profile. Use a new user-data root even for original-mode scene research because normal play may autosave.

First approved code change: a default-off read-only probe module, compiled into an isolated game build and called from existing wrappers. Begin with screen/update transitions (`sub_825A0FD8` / `Native60ScreenUpdateFunction`), game-loop boundaries (`sub_8258E500`), fiber context, and existing minigame/camera observations. Preserve all original calls and timing behavior. Extend only after regenerated code establishes new call contracts.

Manually select Niki Beach in the original game where available. Record activity/location selection, load completion, actor creation, first update, input activation, camera owner, end request and teardown. Compare volleyball, Beach Flags and relaxation entries that actually use this location. Log resource identifiers rather than dumping assets. Trace callers/vtables to distinguish shared environment setup from activity rules and actor setup. No known function in the current evidence can be safely labeled the Niki Beach initializer.

### 2. Environment initialization

Use the smallest **verified** original setup path retaining environment geometry, materials, lighting, audio and required update services. Initially load one existing location variant and fixed original lighting. Select a verified named preset; never send guessed numeric mode/location IDs to guest functions.

Candidate strategies, to choose after traces:

1. Reuse a common environment/character initializer and provide a host-owned session coordinator.
2. Retain an original scene shell while replacing only its proven activity controller and completion policy.

The first strategy is preferable if lifecycle contracts are clear. The second is acceptable only if competition actors, mandatory rules, timers and schedule effects are actually removed from active ownership. Freezing an existing match is a discovery experiment, not the standalone milestone. Do not append values to unknown guest mode tables or bypass allocation/registration/teardown calls.

### 3. Player spawning and animation

Identify actor constructor, registration, outfit/resource dependencies and destructor. Use one verified compatible character with an existing loaded outfit; customization is deferred. Ensure no opponents or partners are created, or prove their registration can be omitted safely. Hiding meshes is insufficient if their AI or activity logic remains active.

Measure the original character transform on proven ground and query support before accepting a fixed spawn. Document position, orientation, world scale, up axis, capsule/foot offset and safe region. No spawn coordinates are known now.

Trace idle/walk/run selection and character update, including `sub_82513930`, keyframe evaluator `sub_825A4550`, root-motion writer `sub_82510218` and activity motion-request leads. Verify skeleton compatibility, loop boundaries, blend duration, root rotation/translation and motion callback requirements. Prefer the original animation system. Decide explicitly whether it supplies displacement; if locomotion owns displacement, handle animation root motion once in a verified narrow path. Never combine native root travel and an extra stick-based translation blindly.

### 4. Input and character movement

Read player 1 input through the verified existing cache/runtime path, respecting device assignment and remaps. Apply radial deadzone and magnitude clamping. Project camera forward/right onto the ground plane, form desired motion from left stick, normalize diagonal input, and scale speed by magnitude. Choose walk/run thresholds and speeds from measured animations/world units; sprint is optional and deferred until normal motion is stable.

Rotate facing toward desired direction with a bounded, cadence-aware angular rate. Maintain explicit idle/walk/run state; pause and disconnect produce zero movement. Let one owner control the actor per simulation update. Preserve existing original-mode logic on every disabled path.

Do not assume all updates receive delta seconds. Determine a verified tick or adapt explicitly to the supported 30/60 profiles. Test animation distance/speed together. No movement should advance from UI draw frequency or display-only interpolated transforms.

### 5. Terrain and collision

Before moving, map existing ground and obstacle tests through regenerated code and activity traces. Distinguish analytical court planes, scripted runner rails, physical scenery meshes and decorative graphics. Start with a contiguous verified beach patch. Record supported surfaces and exclusion boundaries in the evidence ledger.

For each proposed displacement, use the verified collision shape/query to sweep against obstacles, find supporting ground, reject unsupported/too-steep movement, and resolve the final root/foot transform. Validate normals, step height and query masks from measured behavior. If the guest supports only a ground-height query, document its limits and supply a conservative bounded controller; do not claim full obstacle/camera collision without proof.

On no ground, invalid transform or unload, stop movement and recover to the last validated safe transform through the controller's own state. Do not teleport across unverified terrain. If usable collision is absent, stop gameplay implementation and present a scoped authored-collision option and licensing/output plan; no blanket boundary removal.

### 6. Third-person camera

Verify selected camera ownership and publication using slot-10 and `sub_8241AF08` / `sub_82740E08` leads. Initial camera: stable follow target at the actor, bounded pitch, wrapped yaw, measured fixed distance and adjustable sensitivity/inverted Y. Smooth with a response chosen for the verified cadence; avoid lag that feeds back into movement direction unexpectedly.

Right stick changes yaw/pitch; left stick stays character-relative to the resulting ground-projected view. Clamp or reject extreme/nonfinite values. Preserve projection/aspect and current renderer options. Suppress cinematic camera requests only while this session owns the camera. Camera obstruction avoidance must use proven queries; until available, restrict camera distance/area to a verified unobstructed configuration and document that limitation. Full prototype signoff must explicitly assess camera obstruction behavior.

### 7. Scene lifetime, pause and exit

Identify activity completion, scene shutdown, vacation-period advancement and autosave request writers separately. Bypass only verified FreeRoam-local activity-end/progression requests; keep rendering, physics, animation, audio and indispensable fibers alive. Do not suppress `sub_8265E7B8` globally or stop the entire scheduler.

Pause/Menu must provide resume and exit-to-original options. Pause movement and owned camera input while menus are open; decide world simulation pause only after its behavior is verified. A host overlay may queue an exit request, which is processed on the guest owner thread.

Exit disables new movement/interaction, returns camera/input ownership, releases mod-owned actor/resources in traced order, invokes normal scene teardown, and returns to a known original screen without advancing the vacation. Clear all session state after successful teardown. Handle exit during entry, controller disconnect, load failure and unexpected original transitions. Do not restore raw guest snapshots or assume that jumping to the title cleans up resources.

### 8. Test strategy and performance

Build with feature/probe flags OFF, diagnostic-only, then feature ON. Compile/link each implementation milestone and report actual errors. Use focused host tests only for meaningful new pure logic such as schema rejection or nontrivial movement/collision policy; existing rendering tests do not establish free-roam correctness.

Runtime validation must include:

- Original boot, location selection, representative original activities and return, with feature disabled. Compare a normal build to the experimental build's disabled path.
- One actor at the fixed validated spawn; idle/walk/run, analog speed, diagonal travel, smooth facing, turns, starts/stops and animation loop boundaries.
- Right-stick yaw/pitch, invert Y, sensitivity/distance limits, camera tracking, obstruction, paused UI and ownership restoration.
- Ground slopes/edges, obstacle contact, unsupported surfaces, prolonged idle and invalid-query recovery within documented safe bounds.
- No opponents, competition HUD/results, forced timer exit, period/day changes or vacation-ending effects. Verify observed calendar fields and transition requests, not merely absence of a visible clock.
- At least 20 full entry/exit cycles as an initial soak target, exits during loading/pausing, and at least 30 minutes beyond the ordinary activity duration. These finite tests cannot prove mathematical infinity; inspect that no forced completion path remains active.
- Controller remaps, keyboard, disconnect/reconnect and window focus loss; no stuck movement or duplicate pause action.
- Original 30 FPS baseline, then existing supported native 60 FPS profile; compare wall-time movement distance and animation rate. Exercise currently supported renderer choices without changing their implementations.
- File-write audit with isolated user data; original saves/assets unchanged. Profile frame-time distribution, query/log costs and memory growth versus baseline. Investigate any persistent regression before signoff.

Record a test table with build revision/profile, scenario, expected behavior, observed result, evidence/log reference and limitations. Mark unexecuted cases explicitly. A build-only run never satisfies Phase 1 acceptance.

## First implementation tasks and discovery gates

All paths in this table that do not already exist are proposed. Each task should finish before dependent gameplay is enabled.

| Task | Objective / dependencies | Files or systems / proposed change | Validation method | Completion criterion |
|---|---|---|---|---|
| T01 | Establish baseline; no gameplay dependencies | Existing setup/codegen/configure scripts, SDK patch, ignored local game/generated/output paths; build without mod changes | Verify input hash; build original target; run with isolated user-data root at 30 FPS; record toolchain and errors | Reproducible original boot/activity/return and generated bodies available; toolchain discrepancies resolved |
| T02 | Add read-only observation; T01 | Proposed `scene_probe.*`, small CMake options and guarded calls from existing timing wrappers | Build probe OFF/ON; compare normal scene behavior and frame-time impact; bounded logs | Default-off probe observes scene transitions without guest writes or behavior changes |
| T03 | Map Niki Beach lifecycle; T02 | Generated callers/vtables, screen/fiber/resource/actor/camera traces; update evidence documents | Trace repeated original entry/update/end/unload for actual available location/activity combinations | Verified load/teardown path, resource owners, scene identity and return target; remaining dependencies named |
| T04 | Establish terrain, spawn and animation contracts; T03 | Read-only bindings/probes, source inspection of collision and root-motion paths; metadata catalog | Ground query visualization/samples, actor transform logs, idle/walk/run resource observations | Safe fixed spawn/contiguous patch and compatible motion IDs established; no guessed coordinates or APIs |
| T05 | Map schedule/autosave/exit policy; T03 | Calendar/transition/file-write tracing, relevant generated callers | Observe original activity completion and period/day changes in isolated data; trace normal return | Known local termination and progression requests plus safe return contract; original saves untouched |
| T06 | Prove minimal isolated scene lifetime; T04/T05 | Proposed `guest_bindings.*`, `free_roam.*`, minimal configuration flags | Build OFF/ON; load one environment and actor, idle, normal exit repeatedly; inspect actor/resource counts | Exactly one actor, required services alive, no competition or schedule effect; safe teardown before movement |
| T07 | Prove camera ownership; T06 | Proposed `third_person_camera.*`; delegate existing camera hook narrowly | One yaw/pitch change, steady follow, then restore original camera; 30 FPS first | Stable camera and repeatable restoration with no script overwrite |
| T08 | Add grounded locomotion and animation; T04/T06/T07 | Proposed `character_controller.*`; verified input, motion and collision adapters | Walk/run/idle and turns inside safe patch; inspect root delta/final transform and terrain results | Camera-relative analog movement, single displacement owner, valid animation and terrain support |
| T09 | Complete controls/camera/options; T07/T08 | Camera obstruction/settings, pause/resume/exit request handling, mod config | Limits, obstruction, remap/focus/disconnect tests; invalid config rejection | Reliable third-person controls, safe camera and accessible exit with configuration optional |
| T10 | Validate unlimited session and compatibility; T05-T09 | Mode-local completion/progression guards only where still needed; regression/soak evidence | Timers/schedule tests, 20 transition cycles, prolonged session, disabled mode and native60 comparisons, performance/file audit | All Phase 1 acceptance items demonstrated or explicitly outstanding; no original-mode/save regression |

T04 and T05 research can be conducted independently after scene mapping. Within T04, animation and collision annotation can be separate research tracks, but both gate movement. If T06 reveals previously unknown lifetime dependencies, return to mapping rather than layering control over an unstable scene.

Executed follow-up: [SCENE_OWNERSHIP.md](SCENE_OWNERSHIP.md) maps the Niki stage binding chain and original single-slot relaxation path. One normal volleyball exit and a subsequent relaxation entry/completion in the same process passed. The next T03/T04 step is relaxation script/resource activation, completion and cleanup tracing, followed by compatible locomotion mapping. T06 remains gated; the original scripted relaxation scene is not a standalone free-roam mode.

## Later milestone gates

| Phase | Deliverable and exit gate |
|---|---|
| 2 | Reusable interaction detection/alignment/cancel/return; two verified relaxation interactions without session or schedule effects |
| 3 | Enter photo mode, detach camera, hide UI, capture to configured path, restore actor/camera state; advanced optics only after separate proofs |
| 4 | Independent FreeCruise with verified water bounds, original vehicle mechanics, no race objectives/opponents/timer and repeatable exit |
| 5 | Configurable hours/minutes clock, freeze/manual adjustment/persistence and at least one verified smooth visible lighting transition; retain internal periods only where necessary |
| 6 | Mode-local unlimited vacation, optional essential-content economy, optional activity access and separate versioned persistence; crash-safe loading and original saves untouched |
| 7 | Audit Marine Race and other scenes for geometry/detail/collision/coordinates/residency; link locations through safe transitions first; only pursue seamless streaming after measurable proof |

## Current next action

The original Phase 0 review boundary was followed by the user's authorization to proceed. Baseline restoration and the scene probe now exist. Continue T03/T04 ownership, ground-query and animation-contract discovery using the verified map in [RESEARCH_FINDINGS.md](RESEARCH_FINDINGS.md). The next gameplay experiment remains a stationary isolated actor with verified cleanup, gated by those contracts; do not infer traversable terrain from rendered scenery.
