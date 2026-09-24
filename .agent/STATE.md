# Current State

Last updated: 2026-09-24
Current phase: M4 continuation; feature 010 GNU RELRO complete; repository v7 organization complete
Integration branch: `bleeding`
Last merged runtime PR: #32
Runtime baseline commit: `9bb52b1e50bf818f6326424975582372db1353cd`
Completed runtime change: `010-elf32-gnu-relro` is DONE. T001/T002 passed CI #231/#232, T003 real post-relocation sealing passed CI #234, and T004 convergence passed exact-head CI #235 / run `35947449163` at `f24c87026b66838c0742bf61ebafe27fba2f117e`.
Completed maintenance change: `repository-organization-v7` is DONE. The v7.1 project/spec/change migration, modular CMake organization, private ELF32 helper cleanup, and reconciled docs/state passed CI #234 and final exact-head convergence CI #235.
Persistence-only closeout head `38304a2e4fcb3bdb96cd77785256afe135c8653a` passed CI #236 / run `35979083056` in all three required lanes.
Completed maintenance change: `test-infrastructure-cleanup-v7` is DONE. Real-fixture binary loading is shared across 12 integration tests, the four duplicated guest-word readers are centralized under `tests/support/`, and CMake test executable setup is deduplicated while preserving all 30 target identities and 49 CTest names. Exact-head CI #238 / run `35979816632` at `56df3ec5a97add0b96f25e02183bdcfbe2aac1c9` passed Linux 49/49 plus both Android lanes.
Persistence-only test-infrastructure closeout head `0b51069def55cfe3ce35621845d948ea2401a38f` passed CI #239 / run `35980275074` in all three required lanes.
Active maintenance work: `elf32-header-layering-v7` removes an unnecessary ELF32 loader-header dependency from load planning/placement by extracting the shared load-error contract without intended API or runtime behavior changes.

## Working

- CPU execution is isolated behind `src/cpu/` with pinned Dynarmic. ARM/Thumb smoke plus current register/control-flow/memory/stack regressions are green in the baseline CI.
- `memory::GuestMemory` is the engine-independent memory seam.
- `LinearGuestMemory` remains the deterministic callback/correctness implementation.
- `MappedGuestMemory` implements logical 32-bit guest VAs, a contiguous high-host-VA 4 GiB reservation, guest page map/protect/unmap lifecycle, Dynarmic fastmem and callback fallback. `guest_va_allocator` provides deterministic bounded non-mutating free-range search.
- D-0003 remains accepted: guest VAs are independent from host pointer identity.
- D-0004 remains accepted: high-base contiguous fastmem is the preferred first Android acceleration path when available; callbacks remain the correctness fallback.
- The shared runtime produces exactly `liba32android.so`.
- Shared pre-mutation ELF32 validation/layout planning is implemented through `src/elf/elf32_load_plan.*`; it is consumed by both mapping and automatic placement.
- ELF32 mapping is implemented through `src/elf/elf32_loader.*`:
  - ELF32 / little-endian / current-version / `EM_ARM` validation;
  - fixed-address `ET_EXEC`;
  - explicit-base `ET_DYN` with load bias preserving all `PT_LOAD p_align` constraints;
  - validated `PT_LOAD` mapping, file copy, BSS zero-fill, final permissions and loader-owned rollback;
  - guest-only result metadata; no host pointers in loader results;
  - zero-or-one validated non-empty `PT_DYNAMIC` guest range inside a readable `PT_LOAD`, with file/address/containment/file-to-load validation before guest mutation;
  - feature-010 T001 prepares zero-or-more validated `PT_GNU_RELRO` guest ranges with host-page-rounded readable PT_LOAD coverage and additive load-result metadata; loader mapping still leaves original PT_LOAD permissions unchanged until a later explicit seal call.
- Automatic `ET_DYN` guest placement is implemented through `src/elf/elf32_dynamic_placement.*`: deterministic caller-bounded first-fit, host-page and `p_align` congruence preservation, no guest-memory mutation, explicit malformed/non-dynamic/window/overflow/no-space failures, and loader-ready `dynamic_base` output.
- Structural dynamic-array parsing is implemented through `src/elf/elf32_dynamic.*`:
  - consumes only the loader-validated `Elf32DynamicSegment` plus `GuestMemory`;
  - parses 8-byte ELF32 entries as signed raw `d_tag` + raw 32-bit value;
  - preserves unknown tags;
  - retains the first `DT_NULL` and ignores later padding;
  - rejects invalid ranges, non-8-byte file-backed sizes, unreadable guest bytes and unterminated arrays;
  - does not rebase/dereference pointer-like values or begin dynamic linking.
- `src/elf/elf32_linker_metadata.*` validates STRTAB/STRSZ, SYMTAB/SYMENT, main REL/RELSZ/RELENT, SONAME, and ordered NEEDED offsets. Feature 008 T001 prepares additive PLT REL metadata for `DT_JMPREL` / `DT_PLTRELSZ` / `DT_PLTREL=DT_REL`, with a separate guest-only descriptor, checked rebasing/range/readability, and explicit malformed-group/type/size failures. The PLT extension and explicit pinned-fixture no-PLT oracle are VERIFIED by CI #222 required-job results at `9a81ed71a027beb166970bcf137bac9a71112f98`.
- `src/elf/elf32_linker_strings.*` now materializes bounded STRTAB entries plus optional SONAME and ordered/repeated NEEDED names. Every call requires an explicit caller-selected payload ceiling; reads remain through `GuestMemory`, use checked guest-address arithmetic, preserve raw bytes, and never mutate guest memory. Aggregate failure is all-or-nothing.
- `src/elf/elf32_dependency_resolver.*` now acquires host-owned dependency image inputs through a caller-owned provider. It preserves ordered/repeated `DT_NEEDED` occurrences, forwards non-empty name bytes unchanged, distinguishes not-found from provider failure, validates non-empty provider identity/image, enforces explicit dependency-count/per-image/total-image ceilings, and returns no partial successful aggregate on failure. It deliberately does not choose guest bases or map dependency ELF images.
- `src/elf/elf32_dependency_loader.*` now owns one transactional recursive loaded-object graph: provider identity is the per-call object key, ordered/repeated dependency edges are preserved, cycles/shared objects reuse existing mappings, first-seen dependencies are automatically placed and loaded as `ET_DYN`, graph-wide object/depth/occurrence/image/string limits are enforced, and aggregate failure rolls back graph-owned mappings in reverse load order.
- `src/elf/elf32_symbol_lookup.*` now implements bounded SysV/GNU hash metadata interpretation and dynamic-symbol indexing, exact byte-name per-object lookup, checked logical 32-bit guest-value computation, explicit unsupported version/TLS/common/XINDEX/IFUNC boundaries, and deterministic graph-local breadth-first lookup from dependency edges. The layer is read-only and never treats guest values as host pointers.
- `src/elf/elf32_relocation.*` now implements separate bounded main-REL and PLT-REL pipelines. Main `DT_REL` supports `R_ARM_NONE`, `R_ARM_RELATIVE`, `R_ARM_GLOB_DAT`, and `R_ARM_ABS32`; PLT REL accepts only eager `R_ARM_JUMP_SLOT`. Both paths use byte-explicit bounded planning, the same graph-local reference policy, plan-before-write validation, unresolved-weak `S=0`, and reverse rollback without permission broadening. `GLOB_DAT` and `JUMP_SLOT` write `S` without treating the in-place word as an addend.
- `src/elf/elf32_relro.*` implements a separate bounded post-relocation GNU RELRO sealing contract: caller-selected declared-page limits, complete preflight before mutation, overlap deduplication, idempotent read-only pages, RW -> R sealing only, and reverse permission rollback on later protection failure. T002 is VERIFIED by CI #232; T003 real-fixture integration is VERIFIED by CI #234, proving the real GLOB_DAT targets survive sealing and become write-protected while non-RELRO permissions remain unchanged.
- The reproducible real ARMv7/Android loader fixture remains generated with pinned NDK r27d / API 26 inputs. Feature 009 additionally generates a freestanding provider/consumer DSO pair twice byte-identically; the consumer has `DT_NEEDED liba32android_jump_slot_provider.so` and `R_ARM_JUMP_SLOT fixture_import`, and the real integration loads the two-object graph and rewrites that slot to the provider guest symbol value without guest execution.
- Project-local workflow state follows v7: `.agent/project.toml` identifies the project; `.agent/specs/` is accepted current truth; `.agent/changes/` holds substantial-work identity/tasks/evidence; root `specs/` is retained historical feature-era material. Generic workflow/runtime/governance stays centralized in `Millesant/.gpt`.

## Partial / not implemented

- Feature 006 bounded graph-local unversioned symbol resolution is DONE at exact-head CI #207. Version-aware lookup remains intentionally unsupported (version tables are rejected). Feature 007 now implements bounded main-`DT_REL` relocation application; Android search-path/namespace/pathname policy plus process-wide link-map lifetime across graph-loading calls remain NOT IMPLEMENTED.
- Broader ARM relocation/linker compatibility remains PARTIAL beyond the completed feature-007 main-`DT_REL` set. Feature 008 has VERIFIED PLT/JMPREL metadata validation, feature 009 has VERIFIED eager `R_ARM_JUMP_SLOT` application, and feature 010 T001-T003 have VERIFIED GNU RELRO metadata, bounded sealing, and real post-relocation integration. Lazy binding/`DT_PLTGOT`, combined main+PLT atomic application, REL32/COPY/instruction relocations, packed/RELA/RELR forms, version-aware/protected requester semantics, TLS/IFUNC, and process-wide/global-group policy remain NOT IMPLEMENTED.
- Version-aware and process-wide/global-group symbol interposition policy: NOT IMPLEMENTED; bounded graph-local unversioned lookup is implemented.
- RELRO processing for the bounded eager-linker scope is IMPLEMENTED and VERIFIED through T003; feature-level T004 documentation/state closeout still awaits its convergence-head CI gate. TLS processing remains NOT IMPLEMENTED.
- End-to-end execution of the real ARM32 fixture through the runtime on Android: NOT IMPLEMENTED / NOT RUN.
- Actual 16 KiB Android host-page behavior: PARTIAL by architecture — x86_64 Android 15 emulator probe PASS with 4 GiB reservation/commit, exact sampled low-VA `MAP_FIXED_NOREPLACE`, collision `EEXIST`, RW->RX, and generated-code return 42; AArch64 runtime on 16 KiB pages remains NOT RUN.
- Broader Android/vendor/kernel compatibility for the high-base reservation: PARTIAL evidence only.

## Validation

### M4 ELF32 GNU RELRO protection (complete)

T001 loader/load-plan GNU RELRO metadata PASSed exact-head GitHub Actions CI #231 / run `35942933233` at `e3ea30a9445c86546933d008ee5a336bd9e91e8e`. Linux PASSed 47/47 CTest including `elf32_relro_metadata`; Android x86_64 and Android arm64-v8a also PASSed. The pinned fixture exposed one GNU RELRO range at linked VA `0x826c`, memsz `0xd94`, page-rounded mapping size `0x1000`, and remained writable immediately after load, proving the loader does not seal before relocation time.

T002 bounded transactional sealing PASSed exact-head GitHub Actions CI #232 / run `35943552213` at `655f933f46c4bb28e5c36fe34b628b92af1f679b`. Linux PASSed 48/48 CTest including `elf32_relro_seal`; Android x86_64 and Android arm64-v8a also PASSed. Synthetic coverage verifies empty success, caller page limits before mutation, overlap deduplication, RW -> R sealing, post-seal write rejection, idempotence, malformed metadata rejection, and unmapped/unreadable/executable preflight without earlier mutation.

T003 real post-relocation GNU RELRO sealing PASSed exact-head GitHub Actions CI #234 / run `35946857448` at `ff1792457f05bd9dd58740e1b768576b9ad4f1c3`. Linux PASSed 49/49 CTest including `elf32_real_relro_seal`; Android x86_64 and Android arm64-v8a also PASSed. The real fixture is relocated before sealing; both observed GLOB_DAT target values remain intact, RELRO writes are rejected after sealing, and non-RELRO mapped-page permissions remain unchanged.

T004 documentation/spec/change/state convergence PASSed exact-head CI #235 / run `35947449163` at `f24c87026b66838c0742bf61ebafe27fba2f117e`; Linux again PASSed 49/49 CTest and both Android jobs PASSed.

### M4 ELF32 eager JUMP_SLOT relocations (complete)

T001 read-only PLT REL planning/reference resolution PASSed exact-head GitHub Actions CI #226 / run `35938429972` at `fe12b6de747884a18d1214f564559d94937d8974`. Linux A32 smoke, Android x86_64 address-space probe, and Android arm64-v8a cross-build all PASSed. The PLT planner accepts only `R_ARM_JUMP_SLOT`, preserves the separate main-REL type policy, enforces count/place/alignment/read/duplicate bounds, and reuses the bounded graph-local reference policy including strong-failure and weak-zero behavior.

T002 transactional eager application PASSed exact-head GitHub Actions CI #227 / run `35938885569` at `666a15ab2edaebdfa3c0f6817dca30e2e2e7a931`. All three required jobs PASSed. Synthetic coverage locks JUMP_SLOT = S with a non-zero original word ignored semantically, unresolved WEAK = 0, strong pre-write failure, reverse rollback after a later write failure, and explicit rollback-failure reporting while existing main-REL tests remain green.

T003 real ARMv7 provider/consumer fixture plus graph-backed application PASSed exact-head CI #228 / run `35939575947` at `815386149732201ce5b64e1b5ad207079491eb80`. The pinned NDK generated the provider/consumer pair twice byte-identically; `readelf` confirmed `DT_NEEDED liba32android_jump_slot_provider.so` plus `R_ARM_JUMP_SLOT fixture_import`; the dependency loader built the expected two-object graph; independent symbol lookup and PLT resolution agreed on the provider guest value; application rewrote the real slot while preserving mapping permissions and every non-target readable segment byte. Artifact ID `10783439676`, digest `sha256:4a68646d281cb35ceb69586388acd5ce0bbb5e5f316ecd285b1b8c4574bffee7`.

T004 documentation/spec/change/state convergence and the final exact-head feature gate PASSed CI #229 / run `35940125841` at `4b255695a9effbaab4028708cd5e7e5a5e23150e`. Linux PASSed 46/46 CTest including `elf32_real_jump_slot_apply`; Android x86_64 address-space probe and Android arm64-v8a cross-build also PASSed. R1-R14 / AC1-AC12 are reconciled with no recorded semantic gap blocking the bounded eager JUMP_SLOT feature.

### M4 ELF32 PLT REL metadata (complete)

T001 implementation plus the REQUIRED_NOW AC8 real-fixture absence oracle are VERIFIED at GitHub Actions CI #222 / run `35933694619` on `9a81ed71a027beb166970bcf137bac9a71112f98`. The Linux A32 smoke, Android x86_64 address-space probe, and Android arm64-v8a cross-build jobs all completed successfully. Synthetic coverage exercises valid main+PLT metadata, singleton duplicates, every partial PLT group, non-REL `DT_PLTREL`, bad PLT byte size, address/range overflow, unreadable PLT bytes, and zero-length PLT tables; the pinned real fixture explicitly requires no PLT dynamic tags and no published `plt_rel_table`.

T002 documentation/spec/state convergence and the final exact-head feature gate PASSed CI #223 / run `35934340806` at `79e9d8c90824baf76d7ff382661422af17e3cb6e`. Linux A32 smoke, Android x86_64 address-space probe, and Android arm64-v8a cross-build all PASSed. R1-R9 / AC1-AC9 are reconciled; PLT entry decoding, `R_ARM_JUMP_SLOT`, and lazy binding remain outside the completed feature.

### M4 ELF32 ARM REL relocation application (active)

T001 bounded read-only main-`DT_REL` decoding/planning PASSed exact-head GitHub Actions run `35850236188` (#210) at `2b4e9185bac43fe9bb46ddf8c7da9b73e0146837`. Linux PASSed 43/43 CTest including `elf32_relocation_plan` and `elf32_real_relocation_plan`; Android x86_64 address-space probe and Android arm64-v8a cross-build also PASSed. The pinned real fixture plan reports exactly two `R_ARM_GLOB_DAT` entries at linked offsets `0x82cc` / `0x82d0`, symbol indexes 2 / 3, with zero original words. T001 remains read-only and performs no relocation writes.

T002 bounded relocation-reference dynsym decoding/name materialization and graph-local resolution PASSed exact-head GitHub Actions run `35889244367` (#212) at `650d7b262540360ba2395a802ba7d7766566d544`. Linux PASSed 43/43 CTest; Android x86_64 and arm64-v8a also PASSed. Coverage includes symbol-index/name bounds, default-visibility GLOBAL/WEAK references, protected/versioned/TLS/IFUNC/common/XINDEX rejection, graph-local definition selection, strong-not-found failure, unresolved weak `S=0`, nested error preservation, and pinned real-fixture resolution of `fixture_bss` / `fixture_data`. T002 remains read-only and performs no relocation writes.

T003 transactional relocation application PASSed exact-head GitHub Actions run `35890660951` (#214) at `41a93348c29fb884befba5ba8bad51ecf0d49665`. Linux PASSed 44/44 CTest including the dedicated `elf32_relocation_apply` suite; Android x86_64 and arm64-v8a also PASSed. Synthetic coverage proves NONE no-write behavior, RELATIVE B+A modulo 2^32, Android-compatible GLOB_DAT S with a nonzero in-place addend ignored, ABS32 S+A modulo 2^32, all-semantic-checks-before-write, late-write reverse rollback, and explicit injected rollback-failure reporting.

T004 pinned real ARM32 GLOB_DAT application PASSed exact-head GitHub Actions run `35891830738` (#216) at `5d74af22c16a7bc99eee7038dfb9f137b22807c2`. Linux PASSed 45/45 CTest including `elf32_real_relocation_apply`; Android x86_64 and arm64-v8a also PASSed. The test resolves `fixture_bss` / `fixture_data` through feature 006, applies the two main GLOB_DAT entries, verifies target words equal those logical guest values, keeps provider calls at zero, preserves initialized data/BSS, preserves every loaded mapping permission, and detects any changed segment byte outside the two relocation target words.

T005 documentation/state/spec convergence and the final feature-head gate PASSed exact-head GitHub Actions run `35918899544` (#218) at `8efe792cfa58a3f34e02dfe0c8bb01fbc3949766`. Linux PASSed 45/45 CTest including `elf32_relocation_plan`, `elf32_relocation_apply`, `elf32_real_relocation_plan`, and `elf32_real_relocation_apply`; Android x86_64 address-space probe and Android arm64-v8a cross-build also PASSed. R1-R15 / AC1-AC15 are reconciled with no recorded semantic gap blocking feature 007.

### M4 ELF32 symbol resolution (complete)

T001 hash metadata/indexing PASSed exact-head GitHub Actions run `35758444356` (#203) at `45cd3e5a322263f53dc02277a9d0e801849515db`. Linux A32 smoke PASSed 40/40 CTest including `elf32_linker_metadata_collection` and `elf32_symbol_index`; Android x86_64 address-space probe and Android arm64-v8a cross-build also PASSed.

T002 exact-name per-object lookup PASSed exact-head GitHub Actions run `35759553586` (#204) at `5f21c8ed48f458f7f3d909fff39523d9ebf9b7e0`. Linux again PASSed 40/40 CTest with the expanded `elf32_symbol_index`; both Android jobs PASSed.

T003 deterministic graph-local BFS lookup PASSed exact-head GitHub Actions run `35760283793` (#205) at `f3997d037f7f5a29b1666dd9a6f5a566b249a2cc`. Linux PASSed 40/40 CTest including the graph-scope cases in `elf32_symbol_index`; Android x86_64 address-space probe and Android arm64-v8a cross-build also PASSed.

T004 pinned real ARM32 GNU-hash fixture integration PASSed exact-head GitHub Actions run `35837480789` (#206) at `2fdba16a13e34370483701345de2605df06811e6`. Linux PASSed 41/41 CTest including `elf32_real_symbol_lookup` and all neighboring real ELF/linker/dependency fixture tests; Android x86_64 address-space probe and Android arm64-v8a cross-build also PASSed. The fixture resolves `fixture_add`, `fixture_data`, and `fixture_bss` from object 0 through GNU hash, reads back initialized data/BSS through resolved guest values, verifies the function value lies in an executable segment, and confirms lookup leaves loaded mappings/permissions/bytes unchanged.

T005 documentation/state/spec convergence and the final feature-head gate PASSed exact-head GitHub Actions run `35847914558` (#207) at `ad022c2cc569c3175ad1cef0140f964817f5a820`. Linux PASSed 41/41 CTest including `elf32_real_symbol_lookup`; Android x86_64 address-space probe and Android arm64-v8a cross-build also PASSed. Requirements/design/code/tests were reconciled with no unrecorded semantic gap blocking R1-R17 / AC1-AC16.

### M4 recursive ELF32 dependency graph loading

PR #32 final head `1ac47ef59f3570989d6fc07cd187c129cbe76588` PASSed GitHub Actions run `35712896172` (#199). Linux A32 smoke PASSed 39/39 CTest including `elf32_dependency_loading` and `elf32_real_dependency_loading`; Android x86_64 address-space probe and Android arm64-v8a cross-build also PASSed. PR #32 was then squash-merged to `bleeding` as `17c2aa78535adbd2084c9396f525750e10c0eff8`. The squash commit uses tree `ee6ce3fe283d7dac0b9665f4ca89def78ca02081`, identical to the validated PR head tree, so the merged source tree is the exact CI-validated tree.

### M4 automatic ET_DYN guest placement

PR #31 squash-merged to `bleeding` as `2c3be26c05dff81be1f81c9565df5906c521a54c`. T001 guest-VA search PASSed CI #156; T002 shared load-plan refactor PASSed CI #162; T003 automatic placement PASSed CI #166; T004 real ARM32 fixture auto-placement PASSed PR-head CI #169 at `053c6435b902eb0b0f6412b9d63a44e32274d060`. T005 feature-gate CI #173 PASSed at `7295efc6dba56e9642b2f05d80def71e1578eea8`, and persistence-only exact-head CI #175 PASSed at `4b23122b6c6fc8469ae1655fcc2b9461118e5e4e`; all three jobs (Linux A32 smoke, Android x86_64 address-space probe, Android arm64-v8a cross-build) completed successfully. `bleeding` was verified identical to the squash commit immediately after merge.


### Termux crash-test evidence recording

PR #25 is merged to `bleeding` as `ce5e3504765b98ca97405580e26e417e702c68de`. Exact-head GitHub Actions run `35436883811` (#138) at `66427cb7dab12b1f299d3eda5cc6be5ad255d5d0` is PASS: Linux A32 smoke PASS and Android `arm64-v8a` cross-build PASS. The merged evidence records the 2026-09-19 Termux normal-smoke PASS and explicit SIGABRT crash-marker termination. Android tombstone/native-backtrace coexistence remains NOT OBSERVED because no native crash record was captured. No post-merge workflow run was observed for the squash-merge commit during reconciliation.

### Opt-in crash-test diagnostics

PR #23 is merged to `bleeding` as `709e9ce74e58ee12d925b64c8466b9afa58cc4a6`. Exact-head GitHub Actions run `35436087356` (#134) at `4cb17e059b8258f0cf8ce462c58012ab7d55249d` is PASS: Linux A32 smoke PASS and Android `arm64-v8a` cross-build PASS. CI verified the opt-in `--crash-test` paths and markers statically and did not execute the destructive mode. No post-merge workflow run was observed for the squash-merge commit during reconciliation. Real Android crash-marker + SIGABRT process termination is PASS on the 2026-09-19 Termux run; Android tombstone/native-backtrace coexistence is NOT OBSERVED.

### Focused CPU regression slice

PR #21 is merged to `bleeding` as `831f1f7dfdc11fcdcc8dc75170fe815d6141a50c`. Exact-head GitHub Actions run `35434636206` (#130) at `6c1ae8704f4230d9b36f59a4bb4c213a3acbe1bc` is PASS: Linux A32 smoke PASS with 33/33 CTest and Android `arm64-v8a` cross-build PASS. Newly named tests `guest_thumb_branch`, `guest_thumb_call`, `guest_thumb_memory_load_store`, `guest_thumb_stack`, `guest_thumb_svc_exception`, `guest_instruction_fetch_fault`, and `guest_thumb_data_fault` each PASS. No post-merge workflow run was observed for the squash-merge commit during reconciliation.

### Address-space probe metadata correction

PR #19 is merged to `bleeding` as `f350fd0cf5ba3411ea2f566f00ba61907ce9b2e8`. Exact-head GitHub Actions run `35432912229` (#126) at `06043cc7d92a7d4465466adb43e43ebedb766c6e` is PASS: Linux A32 smoke PASS and Android arm64-v8a cross-build PASS. The standalone probe now reports `android.ndk_api` for compile-time target metadata and `android.runtime_sdk` / `android.release` from Android system properties; CI rejects the obsolete `android.api=%d` marker. No post-merge workflow run was observed for the squash-merge commit during reconciliation.

### M4 dependency-resolution feature

PR #17 is merged to `bleeding` as `c1f0f30d6fde7c73c93dec83f5808353a838c856`. T001 provider boundary is PASS on GitHub Actions run `35405652527` (#112). T002 provider-error/resource hardening is PASS on `35405978520` (#116). T003 real-fixture zero-dependency integration is PASS on `35406297624` (#118). T005 exact-head gate is PASS on `35406975309` (#122) at `e2502067ef57c77a6c6c6589f9dc60e4ffc8702f`: Linux A32 smoke PASS with 26/26 CTest including `elf32_dependency_resolution` and `elf32_real_dependency_resolution`; Android arm64-v8a cross-build PASS. No post-merge workflow run was observed for the squash-merge commit during reconciliation.

### M4 linker-string feature

PR #15 is merged to `bleeding` as `dbc329e2205828c97267a2de60ce0771c4173cb6`. T001 bounded STRTAB reading is PASS on GitHub Actions run `35387506251` (#97), T002 SONAME/NEEDED aggregation is PASS on `35391818125` (#99), T003 real-fixture string integration is PASS on `35402559596` (#102), T005 feature gate is PASS on `35402997435` (#106), and persistence-only closeout head `6f8a97833f59aa7153d91523283a64286d68c758` is PASS on `35403334639` (#108). Linux closeout CTest reported 24/24 PASS including `elf32_linker_string_entry` and `elf32_real_linker_strings`; Android arm64-v8a cross-build PASS. No post-merge workflow run was observed for the squash-merge commit at reconciliation time.

### M4 linker-metadata feature

PR #13 is merged to `bleeding` as `1169f4eff1fb4ba35a74f55167b3904f12ff2425`. T001 semantic collection is PASS on GitHub Actions run `35329098071` (#78), T002 rebasing/range validation is PASS on `35329808394` (#84), T003 real-fixture integration is PASS on `35332054239` (#87), and the persistence-only closeout head is PASS on `35333822529` (#93). No post-merge workflow run was observed for the squash-merge commit at reconciliation time.


### Current runtime baseline on `bleeding`

GitHub Actions run `35204765081` (#73) on commit `ac008b2d2a3158ffa4cb285e88cfabacea2ca4a2`: PASS.

Linux job:

- pinned NDK availability: PASS;
- real ARM32 fixture generation: PASS;
- second byte-identical fixture generation: PASS;
- configure/build: PASS;
- exact shared-library filename check: PASS;
- CTest suite: 20/20 PASS;
- synthetic ELF32 loader validation/mapping coverage: PASS through CTest;
- synthetic structural dynamic-array coverage: PASS through CTest;
- real ARM32 fixture loader integration: PASS;
- real ARM32 fixture structural dynamic-array integration: PASS;
- real fixture evidence reports `host_page_size=4096`; actual 16 KiB Android host behavior remains NOT RUN.

Baseline fixture SHA-256: `6c2dbda2dec94eaa022ad09391ed3988c3a41828e5c1c065fc0101e5725b84c2`.

Baseline artifacts from run #73:

- ARM32 loader fixture: ID `10488899792`, digest `sha256:ee16d64d297d47bc43e00959acb5f34aceb8d7ca8229120113754f7bfef83d44`;
- Android runtime: ID `10489292010`, digest `sha256:ccc0a1a939e3de908ee61c9370483a9be477b3df7a2750d6fedfc661d41787a4`;
- Android address-space probe: ID `10489571490`, digest `sha256:afc5f00a5f9d7bbdd02853635b7c1247c837d5cc6a8ab8ffe4337abc8d270730`;
- Android runtime smoke bundle: ID `10489451658`, digest `sha256:ba73cd10af20da585f2388aca93065c8d6be3eac80615ffba9890e843a728519`.

Android `arm64-v8a` job:

- runtime + diagnostics configure/build/link: PASS;
- shared-library and diagnostic marker checks: PASS;
- runtime/probe/runtime-smoke artifact uploads: PASS.

### Real-device evidence boundary

Previously recorded Android/AArch64 evidence proves the mapped-memory/fastmem path on one known Android 16 / SDK 36 Termux environment, including direct fastmem-backed A32 data access and fastmem-fault -> callback fallback. It does not establish universal Android compatibility.

## Current blocker

No x86_64 16 KiB address-space blocker remains on the validated Fedora/KVM environment. Exact-head CI #148 PASSed, all project-owned Android final ELF targets are CI-checked for `PT_LOAD p_align=0x4000`, and the exact-head x86_64 emulator harness PASSed end to end. The remaining 16 KiB evidence gap is AArch64 `liba32android.so` / Dynarmic runtime execution on a real/emulated AArch64 16 KiB Android target.

## Important temporary facts
- T004 adds `tests/elf32_dynamic_placement_real_fixture.cpp` and CI artifact evidence. The pinned NDK-generated ARMv7 fixture is planned through `Elf32LoadPlan`, required to expose `0x4000` load-bias alignment, automatically placed without mutation, then loaded using the exact returned `dynamic_base`. Existing explicit-base fixture tests remain unchanged. PR-head CI #169 PASSed at `053c6435b902eb0b0f6412b9d63a44e32274d060`, including the required real-fixture auto-placement evidence.
- T003 adds `src/elf/elf32_dynamic_placement.{h,cpp}` plus focused tests. It accepts an ARM ELF32 image and explicit guest search window, reuses `Elf32LoadPlan`, rejects non-ET_DYN/malformed images distinctly, derives the required placement congruence, invokes the non-mutating guest-VA search, and returns only an explicit loader-ready `dynamic_base`. PR-head CI #166 PASSed all three jobs.
- T002 introduces `src/elf/elf32_load_plan.{h,cpp}` as the single pre-mutation ARM ELF32 validation/layout planner. `load_elf32` consumes that plan while retaining the explicit `dynamic_base` API. PR-head CI #162 PASSed all three jobs.
- T001 adds `src/memory/guest_va_allocator.{h,cpp}` plus `tests/guest_va_allocator.cpp` and CTest wiring. The primitive is non-mutating low-to-high first-fit over `const MappedGuestMemory&`, with explicit bounded window, page-compatible length/alignment/alignment-offset, checked 32-bit guest-space arithmetic, conflict skipping, and `NoSpace` exhaustion. Exact-head CI #156 PASSed all three jobs.
- PR #30 is merged to `bleeding` as `428a76ca7d8335b0198b7a2e26c736bc8dbe198f`. Exact-head CI #149 PASSed Linux A32 smoke, Android x86_64 address-space probe, and Android arm64-v8a cross-build. The Fedora x86_64 16 KiB probe PASS remains recorded; AArch64 16 KiB runtime execution remains NOT RUN.
- Spec `004-elf32-dynamic-placement` is DONE and merged through PR #31 as `2c3be26c05dff81be1f81c9565df5906c521a54c`. It keeps `load_elf32` explicit-base semantics and adds the shared load plan plus non-mutating automatic ET_DYN placement layer.
- Fedora 16 KiB emulator environment is VALIDATED for the standalone x86_64 address-space/JIT probe at exact commit `dad047a71636974173da6b14c388df09ea58deb9`: `PAGE_SIZE=16384`, Android 15 / SDK 35, kernel `6.6.50-android15-8-g8adecb593e9b-ab12525588`, 4 GiB reserve/commit PASS, sampled exact low-VA mappings + EEXIST collisions PASS, RW->RX PASS, generated-code return 42 PASS, harness final PASS.
- PR #27 is merged to `bleeding` as `3e776e4baaf9862affb2b42fb0f706292cdf179a`; exact-head CI #142 PASS on both Linux A32 smoke and Android arm64-v8a cross-build. No post-merge workflow run was observed during merge verification.
- The user moved the local development host from WSL to native Fedora 44 on x86_64. Native KVM is now the intended PC-emulator path. The next implementation must add an x86_64 standalone Android address-space probe artifact/harness while keeping AArch64 liba32android/Dynarmic validation separate.
- `tools/run_android_16k_validation.sh` now provides the bounded 16 KiB AArch64 emulator/device evidence path; it requires `PAGE_SIZE=16384` and `aarch64`, captures probe/smoke/fallback logs, and never runs `--crash-test`. Exact-head CI and emulator execution are NOT RUN.
- Diagnostic tools have merged opt-in `--crash-test` paths. On 2026-09-19 the runtime-smoke CI #134 artifact emitted the armed/SIGABRT markers and `A32CRASH|...|signal=6|...` before the shell reported `Aborted`; Android tombstone/native-backtrace coexistence remains NOT OBSERVED.
- CPU regression slice adds Thumb branch/call/memory/stack, Thumb SVC exception, instruction-fetch fault, and Thumb data-fault coverage; nearby test comments/setup are cleaned without changing runtime contracts. Exact-head CI #130 PASS with 33/33 CTest and Android cross-build PASS.

- `specs/002-elf32-linker-strings/` is merged through PR #15 as `dbc329e2205828c97267a2de60ce0771c4173cb6`. T001 PASS #97, T002 PASS #99, T003 PASS #102, T005 feature gate PASS #106, and persistence-only closeout PASS #108.
- Dependency loading/search-path policy remains deliberately deferred by merged `002`; `003-elf32-dependency-resolution` is now readiness-checked on `m4-elf32-dependency-resolution`.
- `003` assigns filesystem/search-path/namespace lookup policy to an injected provider, preserves one request/result occurrence per ordered `DT_NEEDED`, requires explicit dependency-count/per-image/total-image byte limits, and stops before guest mapping because `ET_DYN` placement remains explicit.
- `003-elf32-dependency-resolution` is merged through PR #17 as `c1f0f30d6fde7c73c93dec83f5808353a838c856`: T001 PASS #112, T002 PASS #116, T003 PASS #118, T004 DONE, T005 PASS #122.

- `specs/000-current-baseline/` is a documentation conversion of already implemented behavior; the runtime evidence above remains its validation basis.
- User-owned WSL local validation is now available: pinned NDK r27d (`27.3.13750724`) fixture/host build PASS, 26/26 CTest PASS, and Android `arm64-v8a` cross-build PASS were reported on 2026-09-19. The pasted local logs did not include a Git commit identity, so this is environment-capability evidence rather than an exact-commit release gate.
- Real Android/AArch64 runtime behavior still requires Termux/device execution; the user prefers downloading the CI-produced runtime-smoke artifact for those runs.
- Future substantial features use stable `.agent/changes/<change-id>/` records and explicit deltas against accepted `.agent/specs/`; the historical root `specs/` tree is not extended.


- Spec `005-elf32-dependency-loading` is DONE and merged through PR #32 as `17c2aa78535adbd2084c9396f525750e10c0eff8`. Final head CI #199 PASSed all three jobs with 39/39 CTest, including focused recursive graph coverage and pinned real-fixture graph integration. The feature preserves the acquisition-only resolver boundary and adds provider-identity dedup/cycles, deterministic ordered edges, ET_DYN dependency placement/loading, graph-wide limits, and reverse-order rollback.
