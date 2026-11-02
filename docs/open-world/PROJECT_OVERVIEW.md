# DOAX2 Open World: Phase 0 scope

Latest test-access change: [PICTORIAL_RESEARCH.md](PICTORIAL_RESEARCH.md) documents default-off runtime-only Virtual Pictorials access. Main-menu and nine-character access are verified; the user confirmed that pictorials work with the final per-scene override.

> Follow-up: preparation, default-off diagnostic tooling and actual game navigation were subsequently authorized and executed. See [RESEARCH_FINDINGS.md](RESEARCH_FINDINGS.md) for current builds, runtime evidence, verified guest addresses and remaining blockers. Statements below about missing generated code, no execution and pending approval describe the original Phase 0 snapshot only.

Analysis date: 2026-11-01. Source baseline: `23026d262d7b215d30ddfa1d263fef0c3696d945`, branch `feature/open-world`.

## Assessment

Latest research: [TERRAIN_RESEARCH.md](TERRAIN_RESEARCH.md) verifies native Niki collision geometry and describes the experimental grounding trial. Independent actor spawning and camera/controller work remain outstanding.

Follow-up: [SCENE_OWNERSHIP.md](SCENE_OWNERSHIP.md) traces stage terrain binding and documents an original Niki relaxation scene with one observed native rig slot, followed by normal return. This provides a concrete single-character research path; independent spawning and reusable locomotion are still unverified.

Current command findings: [SCRIPT_COMMANDS.md](SCRIPT_COMMANDS.md) verifies native activation, motion selection and camera setup during original relaxation playback. The next step is a safe script wait/control point and per-frame camera evaluation before a separately gated single-actor lifetime experiment.

Current lifetime result: [RELAXATION_HOLD_TRIAL.md](RELAXATION_HOLD_TRIAL.md) demonstrates a finite five-second script hold and normal cleanup. The user may have ended playback with Enter; the termination cause is unconfirmed and requires a repeat without manual input. Extended lifetime is not established. Main-menu pictorial unlock predicates are mapped as the next route to reproducible scene selection, without changing ownership/save data.

The repository supports native hooks around recompiled guest functions, controller input, configurable runtime behavior, and separate development builds. These are useful foundations for an incremental conversion. A controllable Niki Beach scene is **plausible, not established**: the beach's initializer, actor lifecycle, collision queries, and independent scene lifetime have not been mapped. Seamless island traversal is a research objective, not a demonstrated engine capability.

The initial Phase 0 plan was followed by authorization for autonomous research. SDK preparation, code generation, baseline/probe builds and actual game navigation are now complete. Default-off observational tooling exists; no free-roam gameplay has been implemented. The existing user's `INSTRUCTIONS.txt` remains untouched. See the executed findings for test coverage and unresolved contracts.

## Product boundaries

Preserve the game's identity and reuse locally supplied characters, scenery, animations, audio, and activities where verified. Exploration should be optional, relaxing, indefinite, and independent of compulsory economy, competitive activities, relationships, and vacation progression. Original behavior must remain available with the mod disabled.

The first playable milestone is one character walking and running in a verified safe part of Niki Beach, with camera-relative movement, a rotating third-person camera, terrain interaction, no activity-driven termination, and repeatable entry/exit. No NPCs, opponents, interaction framework, photo mode, continuous clock, world streaming, or GUI overhaul belong in that milestone.

## Evidence labels used throughout these documents

- **Confirmed**: directly observed checkout state or source implementation. Source comments documenting earlier experiments are identified as such; their runtime claims were not reproduced here.
- **Hypothesis / proposed**: an engineering approach or likely reuse opportunity requiring validation.
- **Unknown**: no sufficient evidence in this checkout. No address or API is invented to fill the gap.

The four-period calendar is now correlated with UI and its advancement traced. Source comparisons to 56 support the expected 14-day boundary; the full forced-ending path remains untested. Exact findings are in the follow-up report.

## Deliverable map

| Document | Contents |
|---|---|
| [ARCHITECTURE.md](ARCHITECTURE.md) | Runtime boundary, integration proposal, module ownership, flags, configuration and launcher decision |
| [REPOSITORY_ANALYSIS.md](REPOSITORY_ANALYSIS.md) | Checkout/build inventory, source evidence, known function/address leads, missing dependencies |
| [ENGINE_CAPABILITIES.md](ENGINE_CAPABILITIES.md) | Feasibility matrix for every phase, reuse evidence and research experiments |
| [TECHNICAL_RISKS.md](TECHNICAL_RISKS.md) | Risk register, stop conditions and mitigations |
| [DEVELOPMENT_ROADMAP.md](DEVELOPMENT_ROADMAP.md) | Dependencies, detailed Phase 1 specification, verifiable tasks and validation |

## Confirmed foundations

The SDK and recursive dependencies are present, the existing preparation patch is applied locally, and the SDK/code generator, guest C++ and separate baseline/probe builds now exist. The locally supplied executable matches the supported hash. Proprietary assets, generated files, binaries, captures, dumps and experimental saves remain ignored. The SDK preparation changes are not a new recorded submodule revision.

## Hypothesis and unknowns

A host-owned mode coordinator may selectively reuse guest scene and actor functions without adding a new numeric value to an unknown guest mode enum. This is a hypothesis until normal initialization, update ownership, save behavior and teardown are traced. The presence of a hook mechanism alone does not establish safe FreeRoam mode support.

## Recommended first experiment

Continue from the verified Niki scene and calendar traces to resource ownership, actor lifetime, ground queries and portable animation contracts. Compare volleyball, Beach Flags and relaxation only where the original game actually offers that location. Niki Marina is a separate scene name and must not be assumed to mean Niki Beach. The first isolated gameplay experiment should be a stationary single actor with verified cleanup before camera or locomotion work.

Do not suppress timers, force a scene ID, spawn an actor, or write coordinates until those observations establish the relevant contracts. A traced original scene is a discovery milestone; it is not the playable prototype.
