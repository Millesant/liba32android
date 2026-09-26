# Current State

Last updated: 2026-09-25
Integration branch: `bleeding`
Control-plane round: `millesant/.gpt@2b7c8b9245a64560cc9e986d554d99233e34b8c5`
Cleanup implementation revision: `5b1cf991272632ed44d6276d6ec5e982ef732f28`

## Phase

M4 runtime/linker scope is stable through bounded main REL including R_ARM_REL32, eager JUMP_SLOT relocation, one per-object combined main+PLT relocation transaction, GNU RELRO, host execution of the linked real ARM32 fixture, bounded GNU/SysV symbol-version matching, requester/global symbol-scope ordering, a persistent caller-owned ELF32 link map/global group, and validated bounded INIT_ARRAY/FINI_ARRAY metadata decoding. Features `017-elf32-lifecycle-array-metadata`, `016-elf32-link-map-global-group`, `015-elf32-symbol-scope-policy`, `014-elf32-symbol-versioning`, `013-real-arm32-fixture-execution`, `012-elf32-rel32-relocation`, `011-elf32-combined-relocation-transaction`, and repository-wide maintenance change `project-cleanup-v8` are DONE. Feature `018-elf32-init-lifecycle-planning` is ACTIVE: add bounded dependency-first INIT_ARRAY planning with cycle/shared-dependency suppression and sentinel filtering, without invoking guest code.

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
- Linker metadata/strings validate the implemented STRTAB/SYMTAB/main REL/PLT REL/SONAME/NEEDED scope, retain DT_SYMBOLIC plus DF_SYMBOLIC requester-binding metadata, and retain raw DT_FLAGS_1 with explicit DF_1_GLOBAL membership for feature 016 link-map policy.
- Lifecycle metadata validates paired DT_INIT_ARRAY/DT_INIT_ARRAYSZ and DT_FINI_ARRAY/DT_FINI_ARRAYSZ declarations, rebases guest addresses once, validates integral 4-byte entries and readable ranges, and exposes guest-only descriptors. A separate caller-bounded read-only decoder returns raw 32-bit entries in declaration order while preserving null/all-ones sentinels and never executing guest code.
- Feature 018 adds root-scoped dependency-first INIT_ARRAY planning above the raw decoder. Cycles/shared dependencies contribute each object at most once, unique-object and total-entry work are caller-bounded, null/all-ones entries are filtered only at the planning boundary, and retained calls preserve object/entry/function provenance. Focused tests are implemented; exact-head CI verification is pending.
- Dependency acquisition is provider-backed and bounded; recursive one-shot dependency graph loading remains transactional. The caller-owned persistent link map preserves stable object indexes/mappings across root loads, reuses equal identity/image pairs, rejects malformed persistent state before mutation, and rolls back only append-owned state on failure. Its deduplicated global scope is maintained in accumulated object-discovery order from caller-designated global roots plus DF_1_GLOBAL objects and feeds feature-015 reference lookup directly.
- Plain symbol lookup supports bounded SysV/GNU hash indexing and deterministic graph-local breadth-first resolution. Relocation/reference lookup can additionally consume an ordered caller-owned global-scope list from the same graph; ordinary requesters search global then local scope, while DT_SYMBOLIC/DF_SYMBOLIC requesters search self then global then remaining local scope. Version matching applies across the selected ordering and all unique candidates share the existing scope ceiling.
- Main DT_REL relocation supports R_ARM_NONE, R_ARM_RELATIVE, R_ARM_GLOB_DAT, R_ARM_ABS32, and AAELF32 R_ARM_REL32 transactionally, including defining-symbol Thumb T-bit handling.
- PLT REL supports eager R_ARM_JUMP_SLOT transactionally.
- One additive per-object API prepares main and PLT relocation tables before mutation, rejects cross-table duplicate targets, applies main then PLT writes, and rolls back across the combined sequence.
- GNU RELRO metadata and explicit post-relocation sealing are implemented with preflight, deduplication, rollback, and no permission broadening.
- The pinned freestanding NDK ARM32 fixture now executes `fixture_add` on the Linux validation host through the generic A32 CPU adapter after graph loading, combined relocation, BSS initialization, and RELRO sealing; the harness uses bounded guest stack/sentinel mappings and a fixed instruction budget.
- Reproducible ARMv7 loader and JUMP_SLOT fixtures back real ELF integration tests.

## Deferred / partial

Still outside the accepted implementation:

- Android search-path/namespace/pathname/accessibility policy, LD_PRELOAD/RTLD policy, and platform-library provider composition above the persistent link map;
- lazy binding and DT_PLTGOT resolver state;
- broader ARM relocation families, packed/RELA/RELR forms;
- TLS/IFUNC and guest constructor/destructor execution beyond the implemented INIT_ARRAY metadata/decoder/planning boundary;
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

Feature 013 exact-head implementation validation at `e899822ae507d1d3954e670bef7365b3aa1196d4`:

- Linux A32 smoke: check `108016545132` — PASS.
- Android arm64-v8a cross-build: check `108016544793` — PASS.
- Android x86_64 address-space probe: check `108016545125` — PASS.

The Linux gate includes CTest registration plus explicit execution-evidence checks for two relocations, the `0x5a` return sentinel marker, and `fixture.execution.status=PASS`. This is host-side integrated execution evidence only; Android-device/AArch64-16-KiB execution remains unproven.

Feature 014 exact-head implementation validation at `5ba659dbf3ad9328c8e46af4441db3a0c4bb4a26`:

- Linux A32 smoke: check `108040133539` — PASS, including the generated LIBC-versioned two-DSO fixture and version-aware JUMP_SLOT application.
- Android arm64-v8a cross-build: check `108040133332` — PASS.
- Android x86_64 address-space probe: check `108040133467` — PASS.

Feature 015 exact-head implementation validation at `2ed5157504dc9d7affac2290b1a19535b28913f9`:

- GitHub Actions CI run `36193971238` — PASS.
- The exact-head workflow contains the required Linux A32 smoke, Android x86_64 address-space probe, and Android arm64-v8a cross-build jobs; the successful run establishes all three required lanes completed successfully.
- Focused coverage proves DT_SYMBOLIC/DF_SYMBOLIC metadata retention, complete explicit-global-index preflight, ordinary global-before-local ordering, requester-first symbolic binding, shared scope ceilings, and main-REL plus PLT JUMP_SLOT inheritance.

The earlier empty connector polls were a wrapper limitation: the generic exact-head workflow-run surface exposes the successful push run and is the recorded verification source.

Feature 016 exact-head implementation validation at `0c374ff84990ee3d64c06a1037f846d90054e5e4`:

- GitHub Actions CI run `36201652255` (#299) — PASS.
- The successful exact-head workflow covers Linux A32 smoke, Android x86_64 address-space probe, and Android arm64-v8a cross-build.
- Focused persistent-map coverage proves multi-root identity reuse, append-only mapping lifetime, cross-load identity/image mismatch rejection, append rollback preserving prior state, accumulated object limits, full persistent-state preflight, DF_1_GLOBAL/global-root membership, discovery-ordered promotion, and direct feature-015 global-scope consumption.

Feature 017 exact-head implementation validation at `e380b96f4e5d2c81a471d051f568b7c2dbef2c1c`:

- Linux A32 smoke check `108292909594` — PASS.
- Android x86_64 address-space probe check `108292909697` — PASS.
- Android arm64-v8a cross-build check `108292909672` — PASS.
- Focused tests cover metadata pairing/duplicates, checked rebasing, size/range/read failures, empty arrays, caller entry ceilings, raw sentinel preservation, declaration order, and no guest mutation.

Historical feature-level evidence remains available in Git history, completed `.agent/changes/` records, root historical `specs/`, and `docs/research/evidence/`.

## Current blockers / external evidence gaps

- Android native tombstone/backtrace coexistence: BLOCKED on an accessible device environment.
- AArch64 16 KiB Android runtime execution: NOT RUN.
- Project license selection: BLOCKED on maintainer choice.
