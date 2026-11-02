# Development architecture and repository integration plan

> Current implementation update: the default-off scene probe, CMake flag and research build/run/navigation helpers now exist. Other gameplay modules below remain proposals. See [RESEARCH_FINDINGS.md](RESEARCH_FINDINGS.md) for their exact integration and validation. The original proposal below is retained as the Phase 0 architecture snapshot.

Evidence status: proposed architecture grounded in the confirmed source boundaries in [REPOSITORY_ANALYSIS.md](REPOSITORY_ANALYSIS.md). No modules or flags described as proposed below have been implemented.

## Confirmed runtime boundary

```text
Native Windows launcher -> runtime command-line CVars and data paths
  -> src/main.cpp / Doax2App / ReXApp setup
    -> generated image metadata and recompiled PPC functions
      <-> title hooks in src/doax2_60fps.cpp
      <-> SDK kernel, input, audio, filesystem and Xenos graphics runtime
```

ReXGlue provides Xbox runtime services and hook authoring, not an exposed DOAX2 editor or gameplay object model. Collision meshes, actors, animation resources, gameplay cameras and scene state belong to the guest game until research proves otherwise. Graphics transform capture is not a world simulation API.

The current SDK checkout is unpatched; the checked-in SDK patch is part of the intended build architecture. It includes title audio and extensive renderer/runtime changes. Apply/build it through the established setup workflow later; do not reimplement those services in the mod.

## Proposed minimal ownership model

Keep title-specific code in `src/open_world/`. Start with observation, then one coordinator and narrowly scoped guest adapters. Do not create all eleven conceptual managers in advance.

| Proposed file | Initial responsibility | Introduce when |
|---|---|---|
| `src/open_world/scene_probe.h/.cpp` | Bounded observation of screen, scene, fibers, actor/camera changes; no guest writes | First approved code experiment |
| `src/open_world/mod_config.h/.cpp` | Versioned configuration, validation, explicit enablement and safe defaults | First experiment / prototype setup |
| `src/open_world/guest_bindings.h/.cpp` | Verified guest calls, structure views, memory validation and build/hash requirements | Only after discovery; no guessed fields |
| `src/open_world/free_roam.h/.cpp` | Session lifecycle, scene ownership, entry/exit requests and mode-local rules | Scene-lifetime proof |
| `src/open_world/character_controller.h/.cpp` | Camera-relative intent, locomotion/animation ownership, validated ground movement | Actor and collision contracts known |
| `src/open_world/third_person_camera.h/.cpp` | Yaw/pitch/distance, follow target, verified publication and obstruction query | Camera contract known |
| `docs/open-world/` | Evidence ledger, supported locations/resources, test reports | Throughout development |

Extend `CMakeLists.txt` conditionally for these sources. Proposed options: `DOAX2_OPEN_WORLD` OFF and `DOAX2_OPEN_WORLD_DIAGNOSTICS` OFF, with diagnostics able to compile a read-only probe independently. Proposed restart-required CVars: `doax2_open_world_enabled=false`, `doax2_open_world_probe=false`. A diagnostics-only build must reject gameplay enablement. Runtime disablement must take the existing original path without guest writes or persistent hook state.

**Hook composition:** existing symbols such as `sub_8258E500` and the camera/minigame hooks already have definitions. Do not define a second `REX_HOOK_RAW` for the same symbol in another module. Add a small guarded observation/delegation call to the existing wrapper at a verified point, preserving its original-call ordering and timing corrections. Keep most new logic outside the large timing file. Preserve PPC registers, stack and floating-point behavior; merely returning the original register state is not enough if a guest callee needs correctly initialized arguments.

Use the guest simulation thread/fiber for authoritative game mutation. UI/controller commands should queue bounded requests; rendering/UI threads must not write actor or camera memory. Session IDs and validated owners must invalidate stale pointers on every transition. Never retain raw guest pointers across load/unload without proving lifetime.

## Proposed mode lifecycle

```text
Disabled -> Ready -> Entering -> Active -> Exiting -> Ready
                         \-> recovery/normal teardown on failure
```

`Ready` means hooks and resource prerequisites have been verified; it does not mean a scene is loaded. Entry must use a traced initialization sequence with a known caller context. Active mode owns only explicitly delegated input, player controller and camera responsibilities. Keep environment, renderer, audio and required fibers updating. Exit requests go through the proven normal scene shutdown path and return to a verified original screen. Do not restore an arbitrary guest-memory snapshot over live objects.

Capture only understood logical return state. Restore original camera/input ownership and temporary patches before unload; clear per-session animation/root-motion caches after teardown. If safe teardown is not known, do not enter experimental mode. A process restart is useful recovery during research but does not satisfy Phase 1 exit acceptance.

## Proposed configuration contract

Use a separate UTF-8 TOML file, e.g. `config/open-world.toml`, with `schema_version = 1`. TOML is already used by this repository, but a title-target TOML parser/link dependency must be verified rather than assumed. CLI-only probe configuration can precede file support.

```toml
schema_version = 1

[free_roam]
enabled = false
# Only verified named presets may be accepted; do not expose raw scene IDs.
location = "niki_beach"
character = "default"
movement_speed_scale = 1.0

[camera]
sensitivity = 1.0
invert_y = false
# Distance default/range must be chosen after world units are measured.

[diagnostics]
enabled = false
```

This is a proposed schema example, not a file to install now. Reject unsupported schema versions and nonfinite/out-of-range values before activation; log a clear error and keep original mode available. Permit only verified locations/character presets. Do not advertise working Niki Beach selection before that preset exists. Keep configuration distinct from session persistence. Later sections can add photo, cruise and clock options with explicit migrations; use a configurable 60-real-minute day only when Phase 5 exists.

Resolve configuration precedence explicitly: safe defaults < mod configuration < explicit mod CLI overrides. The existing launcher continues to own its existing graphics/timing options. Do not let a mod profile silently replace those options. Write mod files atomically through a temporary file and replacement, preserve a previous valid version, and never rewrite `launcher.ini` or guest saves from the mod configuration layer.

## Input and timing compatibility

Use the runtime's assigned/merged controller path, respecting keyboard/controller remaps, rather than adding another Windows XInput-only controller. Prefer the already sampled guest input if its cache contract is identified. If polling `InputSystem::GetState` is necessary, validate thread ownership and avoid redundant polling. Normalize asymmetric signed stick ranges, apply radial deadzones, clamp magnitude, and zero movement on disconnect/focus loss. Input is consumed only by the owner of the active mode; pause UI must not also move the character.

The public launcher selects 30 FPS or native 60 through `--video_mode_refresh_rate`, `--doax2_engine_60fps`, `--doax2_native60_tick_scale=0.5` and other settings. Source contains per-call timing corrections, script-fiber pacing and root-motion smoothing. First establish behavior at original 30 FPS; then explicitly validate the supported 60 FPS path. Do not layer another global half-step or drive authoritative locomotion from render frames. Measure the verified simulation cadence and preserve original pause semantics.

## Save isolation and logging

Use an isolated `--user_data_root` for experiments so normal guest autosaves cannot touch original user data. Before enabling entry, identify every write path and determine whether session actions schedule autosaves. A separate mod JSON/TOML session file can be designed in Phase 6; it must store logical validated fields, never raw pointers or complete guest memory. Do not copy or modify existing saves without approval.

Reuse `REXLOG_*` with diagnostics disabled by default, bounded buffers and rate-limited samples. Start with transition/fiber/owner changes, then add coordinates, animation ID, camera transform and collision hit summaries only after their fields are known. Do not dump all guest memory per frame, allocate a new world-query buffer per tick, or log account/save contents.

## Launcher / mod manager decision

Confirmed: `src/launcher.cpp` and `src/launcher_public.cpp` are C++ Win32 launchers; the public launcher stores `config/launcher.ini`, launches with explicit game/user-data paths and offers SDL controller settings. There is no established C# application in this integration.

Recommendation: initially launch experiments by CLI/profile; later add a small feature panel to the existing public launcher once the schema and gameplay work. This avoids an immediate WPF/WinUI deployment dependency. A separate C# manager is reasonable only if profile/customization complexity justifies another application. WPF versus WinUI 3 is undecided and requires a later packaging/dependency evaluation. Neither should become required for launching basic gameplay.

## Unknowns / gates

No proposed coordinator can yet invoke verified Niki Beach initialization, player creation, terrain queries, locomotion selection or teardown. The guest binding interface must emerge from research. All module names and source paths above are proposals; no engine capability follows from naming a manager.
