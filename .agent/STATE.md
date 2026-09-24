# Current State

Last updated: 2026-09-24
Integration branch: `bleeding`
Control-plane round: `Millesant/.gpt@609e6cb9cff9d00e241aa5437d9904fc7492f407` (v7.2.0)
Pre-cleanup verified head: `ed8f4d97a521aadb27d29089351737e873b4f5b0`

## Phase

M4 runtime/linker scope is stable through bounded eager JUMP_SLOT relocation and GNU RELRO. The active work is repository-wide maintenance/organization under `project-cleanup-v8`; runtime semantics are intentionally unchanged.

## Active maintenance

`project-cleanup-v8` is IMPLEMENTED pending exact-head validation.

The implementation:

- groups ELF implementation files by loading, metadata, linking, hardening, and private-helper ownership while retaining ELF interface headers under `src/elf/`;
- groups tests into CPU, memory, ELF unit, and ELF integration trees;
- splits CMake test registration into CPU/memory/ELF modules while preserving executable target and CTest names;
- groups Android diagnostics separately from fixture builders under `tools/`;
- updates CI/tool paths and current diagnostics/research references;
- replaces the oversized historical README/state orientation with concise current navigation and adds repository/build/test documentation.

No accepted runtime or ELF contract is intentionally changed by this maintenance round.

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
- Main DT_REL relocation supports R_ARM_NONE, R_ARM_RELATIVE, R_ARM_GLOB_DAT, and R_ARM_ABS32 transactionally.
- PLT REL supports eager R_ARM_JUMP_SLOT transactionally.
- GNU RELRO metadata and explicit post-relocation sealing are implemented with preflight, deduplication, rollback, and no permission broadening.
- Reproducible ARMv7 loader and JUMP_SLOT fixtures back real ELF integration tests.

## Deferred / partial

Still outside the accepted implementation:

- Android search-path/namespace/pathname policy and process-wide link-map lifetime across independent graph loads;
- version-aware/process-wide/global-group symbol interposition;
- lazy binding and DT_PLTGOT resolver state;
- combined main+PLT atomic application;
- broader ARM relocation families, packed/RELA/RELR forms;
- TLS/IFUNC and constructors/destructors;
- dlopen/dlsym/unload semantics;
- libc/JNI/graphics/audio compatibility layers;
- end-to-end execution of the current real ARM32 fixture through the runtime on Android;
- general application/game compatibility.

AArch64 runtime execution on a real/emulated 16 KiB Android host remains NOT RUN. x86_64 Android 15 16 KiB address-space/JIT probing has passed in the recorded Fedora/KVM environment.

## Validation baseline

The pre-cleanup closeout head `ed8f4d97a521aadb27d29089351737e873b4f5b0` passed all three required exact-head checks:

- Linux A32 smoke: check `107817777741` — PASS.
- Android arm64-v8a cross-build: check `107817778038` — PASS.
- Android x86_64 address-space probe: check `107817778031` — PASS.

The cleanup implementation requires a new exact-head run; historical green checks do not validate moved paths or updated CMake/CI references.

Feature-level evidence remains available in Git history, completed `.agent/changes/` records, root historical `specs/`, and `docs/research/evidence/`.

## Current blockers / external evidence gaps

- Android native tombstone/backtrace coexistence: BLOCKED on an accessible device environment.
- AArch64 16 KiB Android runtime execution: NOT RUN.
- Project license selection: BLOCKED on maintainer choice.
