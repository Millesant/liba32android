# Current State

Last updated: 2026-09-26
Integration branch: `bleeding`
Control-plane round: `millesant/.gpt@f4e926e81ad91d13d02a006f4a18a00f66ae0bab`
Latest maintenance result: `project-cleanup-v9@567ab4931d3d0af69edb7680b0a266be7672df64`

## Phase

M4 runtime/linker work is stable through features 011-026. The accepted stack
includes combined main+PLT relocation transactions, R_ARM_REL32, host execution
of the linked real ARM32 fixture, symbol versioning, requester/global scope
ordering, a persistent link map/global group, lifecycle-array decoding and
dependency-first INIT_ARRAY execution, requester-aware/provider-chain
dependency acquisition with exact-name catalogs, resumable A32 SVC state,
bounded game-agnostic host-service dispatch, exact-SVC host-service registry
composition, and the first bounded Android platform service bridge for
`__android_log_write`.

Features `011-elf32-combined-relocation-transaction` through
`026-a32-android-log-write-service` are DONE. `project-cleanup-v8` and
`project-cleanup-v9` are DONE.

## Repository organization

- `src/cpu/`: engine-independent A32 execution contract plus pinned Dynarmic adapter.
- `src/runtime/`: game-agnostic CPU/memory orchestration, including bounded host-service dispatch and exact-SVC registry composition.
- `src/compat/`: platform compatibility adapters above generic runtime seams; currently includes the bounded A32 Android log-write service.
- `src/memory/`: logical 32-bit guest-memory contracts and mapped address space.
- `src/elf/*.h`: ELF layer contracts; implementations are grouped under `loading/`, `metadata/`, `linking/`, `hardening/`, and `internal/`.
- `tests/cpu/`, `tests/runtime/`, `tests/compat/`, `tests/memory/`, and `tests/elf/`: subsystem regressions and real-fixture integration.
- `cmake/tests/`: domain-specific CPU/runtime/compatibility/memory/ELF test registration.
- `tools/android/`: Android probes/validation; `tools/fixtures/`: reproducible ARM32 fixture builders.
- `docs/architecture/` and `docs/development/`: current guidance; `docs/research/` and root `specs/` are evidence/history, not current contract authority.

## Implemented runtime

- Bounded ARM/Thumb execution with exact stop-PC termination, optional exact initial CPSR, exact SVC-immediate reporting, and resumable post-SVC state.
- Bounded game-agnostic host-service dispatch with one total guest-instruction budget, a separate service-call ceiling, explicit service/fault errors, and preserved completed handler side effects.
- Exact-SVC `A32HostServiceRegistry` composition over caller-owned handlers: unknown IDs remain unhandled; ambiguous/null/direct-self matching entries fail before child dispatch; valid children receive the original memory/register/CPSR state and exact immediate.
- Android `__android_log_write` AAPCS32 service bridge with caller-selected SVC ID, bounded GuestMemory tag/text copies, null-tag preservation, pre-sink guest-memory failures, caller-owned host logging policy, and exact signed 32-bit r0 result propagation.
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

- Android filesystem/search-path/namespace/pathname/accessibility policy, RUNPATH/RPATH, LD_PRELOAD/RTLD policy, concrete platform-library catalog contents, and guest compatibility-shim selection/ELF export;
- stable public embedding/runtime API and runtime-wide structured public error contract;
- lazy binding and `DT_PLTGOT` resolver protocol;
- broader ARM relocation families, COPY/instruction relocations, RELA/RELR/Android packed formats;
- protected-reference self-binding beyond the accepted policy, TLS relocations/addressing, and GNU IFUNC execution;
- persisted constructor-called state, PREINIT/legacy INIT policy, FINI/destructor/unload lifecycle;
- `dlopen`/`dlsym`/unload semantics;
- broader libc/JNI/graphics/audio and Android platform compatibility services, including varargs/stack marshalling for log print/vprint;
- end-to-end execution of the current real ARM32 fixture through the runtime on Android;
- general application/game compatibility.

## Validation and repository-truth evidence

Latest behavior-changing result: feature 026 at
`59fa3eba1d53c6679ef6086cd209198ca7ecb4ac`:

- Linux A32 smoke check `108387154388` — PASS.
- Android x86_64 address-space probe check `108387154358` — PASS.
- Android arm64-v8a cross-build check `108387154270` — PASS.

The feature-026 compatibility regression covers ARM SVC -> registry -> dispatcher
composition, r0-r2 call mapping, signed result bits, null tag, exact string
ceilings, and bounded pre-sink failures. It does not claim a guest ELF
`liblog.so` shim or application compatibility.

Cleanup-v9 exact-head validation at
`567ab4931d3d0af69edb7680b0a266be7672df64`:

- Linux A32 smoke check `108379917871` — PASS.
- Android x86_64 address-space probe check `108379918028` — PASS.
- Android arm64-v8a cross-build check `108379917957` — PASS.

Historical cleanup audit counts describe the cleanup revision, not the expanded
post-feature-025/026 source/test tree.

## Current blockers / external evidence gaps

- Android native tombstone/backtrace coexistence: BLOCKED on an accessible device environment.
- AArch64 16 KiB Android runtime execution: NOT RUN.
- Project license selection: BLOCKED on maintainer choice.
