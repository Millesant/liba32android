# Project Context

## Mission

Build a reusable, game-agnostic AArch32 compatibility runtime for AArch64 Android. The intended stack is ARM32 Android ELF -> ELF32 loader -> dynamic-linker semantics -> future ABI/compatibility layers -> A32 execution engine -> AArch64 Android host.

Minecraft PE 0.15.x is a future stress target, not the architecture.

## Non-goals and invariants

- Do not hard-code one application into the generic runtime.
- Guest addresses are logical 32-bit values; guest pointer values are not host pointers.
- Dynarmic stays behind `src/cpu/`.
- `memory::GuestMemory` remains the engine-independent memory seam.
- Fastmem is optional acceleration; callbacks remain the correctness fallback.
- ELF mapping, structural metadata, dynamic-linker semantics, relocation, and post-relocation hardening remain separate layers.
- Do not broaden permissions or compatibility semantics to make a fixture pass.

## Current stack

- Language/build: C++20, CMake 3.24+, Ninja in CI.
- CPU engine: pinned Dynarmic behind the A32 CPU adapter.
- Memory: `LinearGuestMemory` for deterministic correctness tests; `MappedGuestMemory` for logical guest mappings, protection lifecycle, high-base fastmem reservation, and callback fallback.
- ELF loading: shared pre-mutation load planning, explicit mapping, bounded deterministic ET_DYN placement.
- Dynamic-linker scope: structural dynamic entries, validated linker metadata/strings, bounded dependency acquisition, transactional dependency graph loading, SysV/GNU symbol lookup, main REL plus eager JUMP_SLOT relocation, and explicit GNU RELRO sealing.
- Android validation: x86_64 standalone address-space probe plus arm64-v8a runtime/probe cross-build; real AArch64 16 KiB runtime execution remains an evidence gap.

## Dependency direction

```text
ELF image
  -> loading
  -> structural metadata
  -> linker metadata + strings
  -> dependency graph
  -> symbol lookup
  -> relocation
  -> RELRO hardening

GuestMemory -> CPU adapter -> Dynarmic
```

## Repository map

- `src/cpu/`: CPU abstraction and Dynarmic adapter.
- `src/memory/`: guest-memory contracts and address-space implementation.
- `src/elf/*.h`: ELF layer contracts.
- `src/elf/loading/`: load planning, placement, and mapping implementations.
- `src/elf/metadata/`: dynamic/linker metadata and string implementations.
- `src/elf/linking/`: dependency, symbol, and relocation implementations.
- `src/elf/hardening/`: post-relocation hardening implementations.
- `src/elf/internal/`: private ELF helpers.
- `tests/cpu/`, `tests/memory/`, `tests/elf/`: subsystem tests; ELF synthetic and real-fixture tests are separated.
- `tools/android/`: Android diagnostics and runtime-validation harnesses.
- `tools/fixtures/`: reproducible ARM32 fixture builders.
- `cmake/tests/`: domain-specific test registration.
- `docs/architecture/`: current subsystem design.
- `docs/development/`: build/test/layout guidance.
- `docs/research/`: research and environment-specific evidence.
- `.agent/specs/`: accepted current contracts.
- `.agent/changes/`: substantial change records/evidence.
- root `specs/`: historical pre-v7 feature records only.

## Canonical state

- Repository integration branch: `bleeding`.
- Project identity: `.agent/project.toml`.
- Accepted current contracts: `.agent/specs/`.
- Observed current state: `.agent/STATE.md`.
- Dependency-ordered next work: `.agent/NEXT.md`.
- Durable project decisions: `.agent/DECISIONS.md`.
- Repository-specific agent overlay: `AGENTS.md`.
- Generic workflow/control rules: private external `Millesant/.gpt` control plane.

For substantial work, use a stable `.agent/changes/<change-id>/` identity. Historical feature packages under root `specs/` are evidence, not current contract authority.
