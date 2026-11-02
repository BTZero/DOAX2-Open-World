# Engine capabilities and technical feasibility

> Executed follow-up: live Niki volleyball, original movement and normal return were observed. Ground queries outside the court, standalone actor lifetime and portable animation contracts remain unknown; no free-roam capability is claimed. See [RESEARCH_FINDINGS.md](RESEARCH_FINDINGS.md). The matrix below remains the broader feasibility assessment.

Confidence refers to current evidence for delivering the feature, not probability estimates or implementation promises. Difficulty includes reverse engineering. Confirmed source evidence and addresses are cataloged in [REPOSITORY_ANALYSIS.md](REPOSITORY_ANALYSIS.md); proposed module paths are in [ARCHITECTURE.md](ARCHITECTURE.md).

## Feasibility matrix

| Phase / feature | Existing systems potentially reusable | Required new engineering | Difficulty | Dependencies | Major risk | Confidence | Smallest proof of concept |
|---|---|---|---|---|---|---|---|
| 0: architecture and research | ReXApp, hook ABI, manifest, timing probes, build scripts | Evidence ledger, baseline and scene/actor/collision mapping | Medium-high | Patched built SDK, local input layout, generated source | Missing guest semantics | High for source integration; low for game-system identification | Read-only trace of original scene entry/update/exit |
| 1: Niki Beach scene and one actor | Candidate activity loaders, character update/root motion; none mapped for this location | Isolated session lifetime, verified spawn/despawn and ownership | Very high | Phase 0 discovery, scene/actor contracts | Environment tied to match scripts or additional actors | Low | Enter and exit a traced beach scene without changing gameplay, then prove minimal isolated initialization |
| 1: walk/run animation and input | SDK merged input; character/root-motion/volleyball hooks | Camera-relative intent, motion selection, blending and movement arbitration | High | Actor, animation resources, units, ground queries | Script overwrites movement; root motion applied twice | Low-medium for input; low for locomotion | One actor transitions idle/walk/idle within a bounded safe patch |
| 1: follow camera and terrain | Camera fiber/publish probes; minigame collision leads | Orbit control, owner arbitration, ground/sweep and camera obstruction handling | Very high | Camera fields, environment collision contract | Scenery lacks traversable collision | Low | Read-only terrain sampling and isolated camera yaw/restore experiment |
| 2: relaxation interactions | Some live procedural/keyframe animation evidence in historical notes | Resource catalog, alignment, looping, cancellation, reusable interaction state | High | Stable Phase 1, animation/skeleton contract | Pose scene requires props or scripted callbacks | Low-medium | Loop one verified relaxation animation and cancel back to idle; acceptance needs two interactions |
| 3: photo mode | Camera publication, renderer capture diagnostics | Free camera, simulation/render separation, UI hide, screenshot UX, state restoration | High; advanced optics very high | Phase 1 camera and lifecycle; posing benefits from Phase 2 | Global pause also stalls rendering; GPU effects inaccessible | Medium-low for basic camera; low for advanced optics | Detach camera, hide UI, capture one image, restore state |
| 4: jet-ski FreeCruise | Marine Race manager/countdown/input/rider hooks | Separate race rules from vehicle/water update, suppress opponents/checkpoints/HUD safely | High-very high | Phase 1 lifecycle/config patterns, Marine Race teardown map | Race code also owns physics, course loading and recovery | Medium-low | Continue controllable vehicle updates in a verified water patch without a completion timer |
| 5: continuous clock/lighting | Game's period-specific environments may exist; fields not mapped | Host/mod clock, compatibility periods, lighting interpolation, persistence | Very high for visible transitions | Stable exploration, lighting parameter map; persistence format for cross-session time | Baked presets require reload; reflections/shadows disagree | Medium for clock alone; low for lighting acceptance | Blend one verified light parameter in one scene while preserving rendering |
| 6: unlimited vacation, optional economy/activities, persistence | Existing optional winnings multiplier, original activities and save runtime | Mode-local calendar/end conditions, customization access, transition adapters, separate saves | High-very high | Stable lifecycle; calendar/autosave/economy mapping; Phase 5 clock for full persistence | Guest save corruption and end-of-vacation callbacks | Low-medium | Separate mod position/settings round trip, then mode-local period advancement without ending |
| 7: connected island and expanded traversal | Marine Race island scenery and separate activity locations are candidates | Geometry/collision audit, coordinate registration, loading/streaming, residency, swimming if needed | Extreme | Stable earlier modes and proven environment ownership | Distant backdrop meshes and missing collision/detail; memory limits | Low | Measure two scenes' scale/origins/collision and demonstrate a controlled transition before streaming |
| Supporting GUI | Existing C++ launchers and configuration | Stable shared schema, small settings panel; later profiles if needed | Low-medium after systems stabilize | Working feature flags and schema | GUI exposes unsupported options or becomes launch dependency | High for simple launcher additions | One validated setting passed to runtime, no gameplay dependency |

## Per-feature evidence and unknowns

### Scene initialization, actor lifetime and unlimited exploration

**Confirmed:** `src/doax2_60fps.cpp` wraps the screen manager, scene dispatch and minigame updates. `config/fibers.toml` maps runtime fiber replacements. **Hypothesis:** an original beach activity can supply environment and actor initialization independent of its competition state. **Unknown:** scene ID, environment asset members, actor constructor/destructor, valid spawn and transition/save dependencies. Regenerated bodies and event traces must identify exact calls and resource ownership before a dedicated FreeRoam entry is proposed as executable code. Globally stopping a scheduler or countdown could stop loading, animation or teardown.

### Locomotion and animation

**Confirmed:** wrappers access per-player root-motion deltas (`sub_82510218`), character rig/blend updates (`sub_82513930`), keyframe evaluation (`sub_825A4550`) and volleyball motion requests (`sub_8268DAE8`). These are address leads, not exported game APIs. **Hypothesis:** idle/walk/run resources can be selected on one compatible character. **Unknown:** animation archive members, motion-ID meaning outside volleyball, blending rules, bone hierarchy dependencies, root transform authority and whether a move requires activity-specific state.

Local `chr.afs`, `mot.afs`, `stg.afs` and other archives are confirmed filenames only. Their names are clues, not verified content assignments or formats beyond their observed files. No proprietary extraction is needed for this documentation. Later catalog only metadata and observations: scene, resource/member identifier, character/skeleton, duration, loop behavior, root delta, props, entry/exit callbacks and provenance. Never distribute archive contents or derived animations.

### Input, camera and collision

**Confirmed:** runtime input API and camera observation/publish hooks exist. **Hypothesis:** replace the selected camera owner's parameters while preserving its rendering path. **Unknown:** yaw/pitch units, handedness, projection controls, obstruction API and all terrain geometry contracts. No reusable DOAX2 terrain raycast was identified in the SDK interfaces; hash/pipeline references to the word collision are unrelated. Ball collision and render vertex data cannot establish solid ground. Terrain and camera probes must distinguish a hit against a gameplay surface from decorative graphics.

### Relaxation and photographs

Historical procedural/scene notes support live 3D content in some sampled previews/relaxation scenes; local `.sfd` files and movie hooks demonstrate prerecorded content also exists. **Unknown:** which requested lounging, stretching and scenic poses are reusable loops. Each must be classified before reuse.

Screenshot/capture diagnostics are leads for image output, not a complete user photo feature. Capture must exclude HUD/overlays as requested and handle output paths/failures. FOV, roll, pause behavior and animation freeze need guest verification. Depth of field, exposure, grading, lighting and arbitrary bone posing remain research items; do not promise renderer support from camera matrices alone.

### Jet skis and water

`sub_826496A0` is wrapped as race manager; other hooks identify racer countdown and rider/vehicle-related updates. This is stronger reuse evidence than a wholly new vehicle implementation, but there is no verified separation of race rules from physics. Water stepping hooks in the timing file do not prove a world-height query usable by swimming or on-foot shore transitions. Inspect checkpoint, out-of-bounds, opponent, camera and teardown writers. Keep course boundaries until residency and collision are measured beyond them.

### Calendar, economy and persistence

`doax2_money_multiplier` already exists but changes winnings; it does not establish free customization or bypass essential-content locks. Four-period time and the 14-day limit are specified behavior whose fields/functions remain unmapped. Mod saves should be independent, versioned and validated against the current environment. Period compatibility, automatic saving and activity result commits need explicit transition tests. Do not disable every countdown or zero game speed to obtain an unlimited vacation.

### Continuous time and island expansion

A standalone clock is straightforward host engineering, but Phase 5 acceptance also needs visible lighting transition. No verified sun/sky/shadow/reflection control or scene streaming API was found. Phase 7 must measure mesh quality, world coordinates, load ownership, guest address-space pressure and supported collision before choosing a world foundation. Controlled location transitions are an acceptable intermediate result; true seamless traversal remains conditional.

## Independent research opportunities

After approved baseline preparation, asset metadata classification, guest-function annotation, configuration schema design, and Marine Race boundary/lighting research can progress independently of locomotion implementation. They do not justify changing the delivery order. None should ship unverified gameplay or a full mod manager before the first prototype.
