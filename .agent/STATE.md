# Current State

Last updated: 2026-09-26
Integration branch: `bleeding`
Control-plane round: `millesant/.gpt@f4e926e81ad91d13d02a006f4a18a00f66ae0bab`
Latest maintenance result: `project-cleanup-v9@567ab4931d3d0af69edb7680b0a266be7672df64`

## Phase

M4 runtime/linker work is stable through features 011-024. The accepted stack
now includes combined main+PLT relocation transactions, R_ARM_REL32, host
execution of the linked real ARM32 fixture, symbol versioning, requester/global
scope ordering, a persistent link map/global group, lifecycle-array decoding
and dependency-first INIT_ARRAY execution, requester-aware/provider-chain
dependency acquisition with exact-name catalogs, resumable A32 SVC state, and
bounded game-agnostic host-service dispatch.

Features `011-elf32-combined-relocation-transaction` through
`024-a32-host-service-dispatch` are DONE. `project-cleanup-v8` is DONE.
`project-cleanup-v9` is DONE. It reconciled canonical state/current docs,
historical-spec orientation, and repository-registration evidence without
changing accepted runtime semantics.

## Repository organization

- `src/cpu/`: engine-independent A32 execution contract plus pinned Dynarmic adapter.
- `src/runtime/`: game-agnostic CPU/memory orchestration, including bounded host-service dispatch.
- `src/memory/`: logical 32-bit guest-memory contracts and mapped address space.
- `src/elf/*.h`: ELF layer contracts; implementations are grouped under `loading/`, `metadata/`, `linking/`, `hardening/`, and `internal/`.
- `tests/cpu/`, `tests/runtime/`, `tests/memory/`, and `tests/elf/`: subsystem regressions and real-fixture integration.
- `cmake/tests/`: domain-specific CPU/runtime/memory/ELF test registration.
- `tools/android/`: Android probes/validation; `tools/fixtures/`: reproducible ARM32 fixture builders.
- `docs/architecture/` and `docs/development/`: current guidance; `docs/research/` and root `specs/` are evidence/history, not current contract authority.

## Implemented runtime

- Bounded ARM/Thumb execution with exact stop-PC termination, optional exact initial CPSR, exact SVC-immediate reporting, and resumable post-SVC state.
- Bounded game-agnostic host-service dispatch with one total guest-instruction budget, a separate service-call ceiling, explicit service/fault errors, and preserved completed handler side effects.
- `GuestMemory` engine boundary with deterministic `LinearGuestMemory` and mapped `MappedGuestMemory`, including logical guest VAs, protection lifecycle, optional high-base fastmem, and callback fallback.
- Validated ARM little-endian ELF32 `ET_EXEC`/`ET_DYN` loading, shared pre-mutation planning, bounded automatic placement, structural `PT_DYNAMIC` parsing, and explicit GNU RELRO sealing.
- Linker metadata/string handling for the accepted STRTAB/SYMTAB/hash/main REL/PLT REL/SONAME/NEEDED/version/lifecycle/DT_SYMBOLIC/DF_SYMBOLIC/DF_1_GLOBAL scope.
- Caller-bounded raw INIT_ARRAY/FINI_ARRAY decoding; dependency-first INIT_ARRAY planning; bounded ARM/Thumb INIT_ARRAY execution.
- Provider-backed dependency acquisition with exact requester context, strict ordered provider chains, exact-name caller-owned catalogs, transactional recursive loading, and a persistent cross-root link map/global group.
- Bounded SysV/GNU symbol lookup with version matching and ordinary/global/requester-first symbolic scope ordering.
- Transactional main `DT_REL` support for `R_ARM_NONE`, `R_ARM_RELATIVE`, `R_ARM_GLOB_DAT`, `R_ARM_ABS32`, and `R_ARM_REL32`, plus eager `R_ARM_JUMP_SLOT`.
- Combined per-object main+PLT preflight/write/rollback and explicit post-relocation GNU RELRO hardening.
- Reproducible ARMv7 fixtures and host-side integrated execution of the linked real fixture through load, dependency graph, relocation, BSS, RELRO, and A32 execution.

## Deferred / partial

Still outside the accepted implementation:

- Android filesystem/search-path/namespace/pathname/accessibility policy, RUNPATH/RPATH, LD_PRELOAD/RTLD policy, concrete platform-library catalog contents, and compatibility-shim selection/implementation;
- stable public embedding/runtime API and runtime-wide structured public error contract;
- lazy binding and `DT_PLTGOT` resolver protocol;
- broader ARM relocation families, COPY/instruction relocations, RELA/RELR/Android packed formats;
- protected-reference self-binding beyond the accepted policy, TLS relocations/addressing, and GNU IFUNC execution;
- persisted constructor-called state, PREINIT/legacy INIT policy, FINI/destructor/unload lifecycle;
- `dlopen`/`dlsym`/unload semantics;
- libc/JNI/graphics/audio compatibility services and AAPCS stack marshalling for host services;
- end-to-end execution of the current real ARM32 fixture through the runtime on Android;
- general application/game compatibility.

## Validation and repository-truth evidence

Latest behavior-changing result: feature 024 at
`d4e7b480e28d13e8edc5dd1ccf28abbc3072a7a1`:

- Linux A32 smoke check `108377362582` — PASS.
- Android x86_64 address-space probe check `108377362552` — PASS.
- Android arm64-v8a cross-build check `108377362391` — PASS.

Cleanup-v9 audit evidence:

- complete non-truncated repository-tree/CMake comparison: 17 source `.cpp`
  files and 35 test `.cpp` files, with zero unreferenced source/test `.cpp`
  files;
- 35 current test executable identities and 55 unique CTest names, with no
  duplicate registrations;
- bounded searches found zero TODO, FIXME, placeholder, DISABLED_, GTEST_SKIP,
  not_implemented, assert(false), or logic-error placeholder patterns in the
  implementation surface;
- historical root numbered specs may contain stale ACTIVE/NOT-RUN snapshot
  text by design; `specs/README.md` now makes that provenance boundary
  explicit instead of treating those snapshots as current work.

Per-feature exact-head evidence lives in completed
`.agent/changes/<change-id>/evidence.toml`, current architecture docs, and
`docs/research/evidence/`; it is not duplicated feature-by-feature here.

Cleanup-v9 exact-head validation at
`567ab4931d3d0af69edb7680b0a266be7672df64`:

- Linux A32 smoke check `108379917871` — PASS.
- Android x86_64 address-space probe check `108379918028` — PASS.
- Android arm64-v8a cross-build check `108379917957` — PASS.

The cleanup diff contains no `src/`, `tests/`, `cmake/`, or `tools/`
changes; the green matrix therefore validates the reconciled repository state
without claiming new runtime behavior.

## Current blockers / external evidence gaps

- Android native tombstone/backtrace coexistence: BLOCKED on an accessible device environment.
- AArch64 16 KiB Android runtime execution: NOT RUN.
- Project license selection: BLOCKED on maintainer choice.
