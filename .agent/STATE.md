# Current State

Last updated: 2026-09-26
Integration branch: `bleeding`
Control-plane round: `millesant/.gpt@f4e926e81ad91d13d02a006f4a18a00f66ae0bab`
Latest maintenance result: `project-cleanup-v9@567ab4931d3d0af69edb7680b0a266be7672df64`
Active change: `026-a32-android-log-write-service` — IMPLEMENTED, exact-head validation NOT RUN.

## Phase

M4 runtime/linker work is stable through features 011-025. The accepted stack
includes combined main+PLT relocation transactions, R_ARM_REL32, host execution
of the linked real ARM32 fixture, symbol versioning, requester/global scope
ordering, a persistent link map/global group, lifecycle-array decoding and
dependency-first INIT_ARRAY execution, requester-aware/provider-chain
dependency acquisition with exact-name catalogs, resumable A32 SVC state,
bounded game-agnostic host-service dispatch, and exact-SVC host-service registry
composition.

Features `011-elf32-combined-relocation-transaction` through
`025-a32-host-service-registry` are DONE. Feature
`026-a32-android-log-write-service` has implementation/tests/docs prepared but
is not accepted until its exact-head Linux A32 smoke and both required Android
checks pass. `project-cleanup-v8` and `project-cleanup-v9` are DONE.

## Repository organization

- `src/cpu/`: engine-independent A32 execution contract plus pinned Dynarmic adapter.
- `src/runtime/`: game-agnostic CPU/memory orchestration, including bounded host-service dispatch and exact-SVC registry composition.
- `src/compat/`: platform compatibility adapters above the generic runtime seams; feature 026 currently adds the Android log-write bridge implementation awaiting exact-head validation.
- `src/memory/`: logical 32-bit guest-memory contracts and mapped address space.
- `src/elf/*.h`: ELF layer contracts; implementations are grouped under `loading/`, `metadata/`, `linking/`, `hardening/`, and `internal/`.
- `tests/cpu/`, `tests/runtime/`, `tests/compat/`, `tests/memory/`, and `tests/elf/`: subsystem regressions and real-fixture integration.
- `cmake/tests/`: domain-specific CPU/runtime/compatibility/memory/ELF test registration.
- `tools/android/`: Android probes/validation; `tools/fixtures/`: reproducible ARM32 fixture builders.
- `docs/architecture/` and `docs/development/`: current guidance; `docs/research/` and root `specs/` are evidence/history, not current contract authority.

## Implemented runtime

Accepted behavior through feature 025:

- Bounded ARM/Thumb execution with exact stop-PC termination, optional exact initial CPSR, exact SVC-immediate reporting, and resumable post-SVC state.
- Bounded game-agnostic host-service dispatch with one total guest-instruction budget, a separate service-call ceiling, explicit service/fault errors, and preserved completed handler side effects.
- Exact-SVC `A32HostServiceRegistry` composition over caller-owned handlers: unknown IDs remain unhandled; ambiguous/null/direct-self matching entries fail before child dispatch; valid children receive the original memory/register/CPSR state and exact immediate.
- `GuestMemory` engine boundary with deterministic `LinearGuestMemory` and mapped `MappedGuestMemory`, including logical guest VAs, protection lifecycle, optional high-base fastmem, and callback fallback.
- Validated ARM little-endian ELF32 `ET_EXEC`/`ET_DYN` loading, shared pre-mutation planning, bounded automatic placement, structural `PT_DYNAMIC` parsing, and explicit GNU RELRO sealing.
- Linker metadata/string handling for the accepted STRTAB/SYMTAB/hash/main REL/PLT REL/SONAME/NEEDED/version/lifecycle/DT_SYMBOLIC/DF_SYMBOLIC/DF_1_GLOBAL scope.
- Caller-bounded raw INIT_ARRAY/FINI_ARRAY decoding; dependency-first INIT_ARRAY planning; bounded ARM/Thumb INIT_ARRAY execution.
- Provider-backed dependency acquisition with exact requester context, strict ordered provider chains, exact-name caller-owned catalogs, transactional recursive loading, and a persistent cross-root link map/global group.
- Bounded SysV/GNU symbol lookup with version matching and ordinary/global/requester-first symbolic scope ordering.
- Transactional main `DT_REL` support for `R_ARM_NONE`, `R_ARM_RELATIVE`, `R_ARM_GLOB_DAT`, `R_ARM_ABS32`, and `R_ARM_REL32`, plus eager `R_ARM_JUMP_SLOT`.
- Combined per-object main+PLT preflight/write/rollback and explicit post-relocation GNU RELRO hardening.
- Reproducible ARMv7 fixtures and host-side integrated execution of the linked real fixture through load, dependency graph, relocation, BSS, RELRO, and A32 execution.

Current unverified feature-026 implementation:

- an Android-specific `__android_log_write` service adapter under `src/compat/` consumes r0-r2 according to AAPCS32 and returns the sink's signed 32-bit result bits in r0;
- tag/text strings are copied only through `GuestMemory` under independent caller-provided payload ceilings; null tag is preserved, while null/unreadable/unterminated text fails before sink effects;
- the caller owns logging policy through an abstract sink and selects the exact SVC immediate; no host `liblog` dependency or guest ELF `liblog.so` shim is introduced;
- compatibility regressions are registered, including ARM SVC -> feature-025 registry -> feature-024 dispatcher -> log service composition, but are NOT RUN on the implementation revision yet.

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

Latest accepted behavior-changing result: feature 025 at
`c2fb94e450619378bb0b77880fb0454e53ce38b3`:

- Linux A32 smoke check `108385436541` — PASS.
- Android x86_64 address-space probe check `108385436452` — PASS.
- Android arm64-v8a cross-build check `108385436554` — PASS.

Feature 026 exact-head validation: NOT RUN. Its implementation revision is not
accepted until the required matrix reports terminal success.

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
