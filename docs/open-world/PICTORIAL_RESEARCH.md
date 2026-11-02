# Runtime-only Virtual Pictorials access

The diagnostics build now has restart-required CVar `doax2_open_world_pictorial_unlock=false`. It exposes the title entry, all nine characters and native scene lists without setting collection, ownership or viewed-scene flags. Normal builds omit the implementation; other research modes explicitly leave it disabled.

```powershell
.\scripts\build-open-world-research.ps1 -Mode Probe -Jobs 8
.\scripts\run-open-world-research.ps1 -Mode PictorialResearch
```

The helper uses the existing isolated `out/open-world-research/userdata` namespace. Terrain and five-second hold trials are disabled in this mode. The feature is a research convenience, not a persistent unlock or free-roam mode.

## Verified implementation boundaries

- `sub_82410698` controls title-entry visibility at return address `0x825A0110` and character availability at `0x826EEA58`. The original predicate can cache a collection-completion bit. The override returns true for character indices 0..8 at these two callers without running that mutating predicate. All other calls retain the original native60 wrapper and guest call.
- Individual pictorials use viewed-scene discovery flags. Read-only predicate `sub_826EE060` and its callee `sub_826EDAE8` check those flags. The wrapper retains the original call, then returns true only at the two verified list builders (`0x826EE050`, `0x826EE468`) for bounded character/category/entry indices.
- Entry bounds come from native count table `0x82AA936C`. Its nine category counts are 21, 22, 21, 22, 23, 21, 21, 22 and 23. Native compatibility filtering and resource loading remain in place. The override does not create nonexistent scene records.

## Validation checkpoint

Probe and diagnostics-off Baseline builds compiled and linked successfully after title/character integration. The final per-scene predicate addition also compiled and linked in Probe; it is absent from Baseline's source list.

Run `pictorialresearch-20261102-043142-561.log`, PID 16988, visually confirmed the main-menu **Virtual Pictorials** entry and its nine-character selector. The log confirmed availability overrides for all nine character indices. The first playlist inspection still showed undiscovered scenes as question marks, exposing the separate per-scene gate; the final implementation covers that gate as well. The user then tested the final build and confirmed that pictorials work. This is user-confirmed runtime validation, not a claim that the agent exhaustively played every clip.

Research `rds.dat` and `ups.dat` SHA-256 hashes remained unchanged during the title/character/initial playlist checks, compared with the captured pre-test hashes. The implementation itself performs no ownership/discovery writes. Captures, save-hash records and logs remain ignored under `out/`.

The earlier five-second hold's early exit must not be attributed to clock desynchronization: the user reported possibly pressing Enter and ending the scene. Its cause is unconfirmed; repeat without manual input before drawing a causal conclusion. The hold mode remains separate from this menu-access mode until the pictorial playback descriptor and state block have been mapped.

## Outstanding checks

User validation confirms working pictorial access. Remaining regression work includes disabled-mode visibility, a final save-hash comparison after playback, and representative multi-character/scene coverage. Use deterministic scene selection to repeat actor/camera/hold experiments. No proprietary assets, generated game bodies, binaries, captures or saves are part of this change.
