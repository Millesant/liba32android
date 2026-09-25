# Current State

Last updated: 2026-09-25
Integration branch: `bleeding`
Control-plane round: `millesant/.gpt@2b7c8b9245a64560cc9e986d554d99233e34b8c5`
Cleanup implementation revision: `5b1cf991272632ed44d6276d6ec5e982ef732f28`

## Phase

M4 runtime/linker scope is stable through bounded main REL including R_ARM_REL32, eager JUMP_SLOT relocation, one per-object combined main+PLT relocation transaction, and GNU RELRO. Features `012-elf32-rel32-relocation`, `011-elf32-combined-relocation-transaction`, and repository-wide maintenance change `project-cleanup-v8` are DONE.

## Repository organization

The current tree is organized by ownership:

- ELF interface headers remain under `src/elf/`; implementations are grouped under `loading/`, `metadata/`, `linking/`, `hardening/`, and `internal/`.
- Tests are grouped under `tests/cpu/`, `tests/memory/`, and `tests/elf/` with separate unit/integration/fixture/support ownership.
- CMake test registration is split under `cmake/tests/` by CPU, memory, and ELF.
- Android diagnostics live under `tools/android/`; reproducible fixture builders live under `tools/fixtures/`.
- Documentation has a navigation index plus development layout/build guidance; README and agent orientation are compact current-state entry points.

Persisted inspection at `5b1cf991272632ed44d6276d6ec5e982ef732f28` confirmed 30 test executable identities and 49 CTest names were preserved exactly.

## Implemented runtime

- A32 ARM/Thumb execution is isolated behind `src/cpu/` with pinned Dynarmic.
- `memory::GuestMemory` is the engine-independent memory seam.
- `LinearGuestMemory` provides deterministic correctness behavior.
- `MappedGuestMemory` provides logical 32-bit guest mappings, map/protect/unmap lifecycle, high-base 4 GiB reservation support, Dynarmic fastmem, and callback fallback.
- Guest VAs remain independent from host pointer identity; D-0003 and D-0004 remain accepted.
- The shared target produces exactly `liba32android.so`.
- ELF32 supports validated ARM little-endian ET_EXEC and explicit-base ET_DYN mapping with rollback.
- Shared load planning and deterministic bounded ET_DYN automatic placement are implemented.
- PT_DYNAMIC parsing is structural and guest-memory based.
- Linker metadata/strings validate the implemented STRTAB/SYMTAB/main REL/PLT REL/SONAME/NEEDED scope.
- Dependency acquisition is provider-backed and bounded; recursive dependency graph loading is transactional.
- Symbol lookup supports bounded SysV/GNU hash indexing and graph-local exact-name resolution.
- Main DT_REL relocation supports R_ARM_NONE, R_ARM_RELATIVE, R_ARM_GLOB_DAT, R_ARM_ABS32, and AAELF32 R_ARM_REL32 transactionally, including defining-symbol Thumb T-bit handling.
- PLT REL supports eager R_ARM_JUMP_SLOT transactionally.
- One additive per-object API prepares main and PLT relocation tables before mutation, rejects cross-table duplicate targets, applies main then PLT writes, and rolls back across the combined sequence.
- GNU RELRO metadata and explicit post-relocation sealing are implemented with preflight, deduplication, rollback, and no permission broadening.
- Reproducible ARMv7 loader and JUMP_SLOT fixtures back real ELF integration tests.

## Deferred / partial

Still outside the accepted implementation:

- Android search-path/namespace/pathname policy and process-wide link-map lifetime across independent graph loads;
- version-aware/process-wide/global-group symbol interposition;
- lazy binding and DT_PLTGOT resolver state;
- broader ARM relocation families, packed/RELA/RELR forms;
- TLS/IFUNC and constructors/destructors;
- dlopen/dlsym/unload semantics;
- libc/JNI/graphics/audio compatibility layers;
- end-to-end execution of the current real ARM32 fixture through the runtime on Android;
- general application/game compatibility.

AArch64 runtime execution on a real/emulated 16 KiB Android host remains NOT RUN. x86_64 Android 15 16 KiB address-space/JIT probing has passed in the recorded Fedora/KVM environment.

## Validation

Exact-head cleanup validation at `5b1cf991272632ed44d6276d6ec5e982ef732f28`:

- Linux A32 smoke: check `107823867033` — PASS.
- Android arm64-v8a cross-build: check `107823867078` — PASS.
- Android x86_64 address-space probe: check `107823866703` — PASS.

The cleanup changes only repository organization, build/test registration structure, tooling paths, and documentation. These passing checks establish that the existing CI matrix still builds/tests the reorganized tree; they do not establish new runtime compatibility behavior.

Feature 011 exact-head implementation validation at `600fad8edc3ac2a8f64fc2607e088b263adca902`:

- Linux A32 smoke: check `107911342972` — PASS.
- Android arm64-v8a cross-build: check `107911343141` — PASS.
- Android x86_64 address-space probe: check `107911343155` — PASS.

The combined-transaction coverage proves both-table preflight, deterministic main-then-PLT application, cross-table duplicate rejection, rollback of an earlier main write after a PLT failure, and explicit cross-table rollback failure while preserving the existing single-table APIs.

Feature 012 exact-head implementation validation at `ee4d2b364244fd842b059dd5c254de01709c65f9`:

- Linux A32 smoke: check `108000309377` — PASS.
- Android arm64-v8a cross-build: check `108000309663` — PASS.
- Android x86_64 address-space probe: check `108000309608` — PASS.

The REL32 coverage proves ordinary ARM data relocation, defining Thumb-function T-bit handling with a discriminating addend, and unresolved-weak `S=0,T=0` behavior through the existing transactional write path.

Historical feature-level evidence remains available in Git history, completed `.agent/changes/` records, root historical `specs/`, and `docs/research/evidence/`.

## Current blockers / external evidence gaps

- Android native tombstone/backtrace coexistence: BLOCKED on an accessible device environment.
- AArch64 16 KiB Android runtime execution: NOT RUN.
- Project license selection: BLOCKED on maintainer choice.
