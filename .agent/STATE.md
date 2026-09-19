# Current State

Last updated: 2026-09-19
Current phase: post-M4 maintenance; opt-in crash-test diagnostics merged
Integration branch: `bleeding`
Last merged runtime PR: #23
Runtime baseline commit: `709e9ce74e58ee12d925b64c8466b9afa58cc4a6`
Active runtime work: none; next meaningful validation is real-device `--crash-test` execution from the CI runtime-smoke artifact

## Working

- CPU execution is isolated behind `src/cpu/` with pinned Dynarmic. ARM/Thumb smoke plus current register/control-flow/memory/stack regressions are green in the baseline CI.
- `memory::GuestMemory` is the engine-independent memory seam.
- `LinearGuestMemory` remains the deterministic callback/correctness implementation.
- `MappedGuestMemory` implements logical 32-bit guest VAs, a contiguous high-host-VA 4 GiB reservation, guest page map/protect/unmap lifecycle, Dynarmic fastmem and callback fallback.
- D-0003 remains accepted: guest VAs are independent from host pointer identity.
- D-0004 remains accepted: high-base contiguous fastmem is the preferred first Android acceleration path when available; callbacks remain the correctness fallback.
- The shared runtime produces exactly `liba32android.so`.
- ELF32 mapping is implemented through `src/elf/elf32_loader.*`:
  - ELF32 / little-endian / current-version / `EM_ARM` validation;
  - fixed-address `ET_EXEC`;
  - explicit-base `ET_DYN` with load bias preserving all `PT_LOAD p_align` constraints;
  - validated `PT_LOAD` mapping, file copy, BSS zero-fill, final permissions and loader-owned rollback;
  - guest-only result metadata; no host pointers in loader results;
  - zero-or-one validated non-empty `PT_DYNAMIC` guest range inside a readable `PT_LOAD`, with file/address/containment/file-to-load validation before guest mutation.
- Structural dynamic-array parsing is implemented through `src/elf/elf32_dynamic.*`:
  - consumes only the loader-validated `Elf32DynamicSegment` plus `GuestMemory`;
  - parses 8-byte ELF32 entries as signed raw `d_tag` + raw 32-bit value;
  - preserves unknown tags;
  - retains the first `DT_NULL` and ignores later padding;
  - rejects invalid ranges, non-8-byte file-backed sizes, unreadable guest bytes and unterminated arrays;
  - does not rebase/dereference pointer-like values or begin dynamic linking.
- `src/elf/elf32_linker_metadata.*` now validates the first linker-facing metadata set: STRTAB/STRSZ, SYMTAB/SYMENT, REL/RELSZ/RELENT, SONAME, and ordered NEEDED offsets. Pointer-like STRTAB/SYMTAB/REL values are rebased exactly once with checked 32-bit arithmetic; declared guest ranges are validated read-only through `GuestMemory`; malformed duplicates/groups, entry sizes, REL sizes, offsets, overflows, and unreadable ranges are rejected.
- `src/elf/elf32_linker_strings.*` now materializes bounded STRTAB entries plus optional SONAME and ordered/repeated NEEDED names. Every call requires an explicit caller-selected payload ceiling; reads remain through `GuestMemory`, use checked guest-address arithmetic, preserve raw bytes, and never mutate guest memory. Aggregate failure is all-or-nothing.
- `src/elf/elf32_dependency_resolver.*` now acquires host-owned dependency image inputs through a caller-owned provider. It preserves ordered/repeated `DT_NEEDED` occurrences, forwards non-empty name bytes unchanged, distinguishes not-found from provider failure, validates non-empty provider identity/image, enforces explicit dependency-count/per-image/total-image ceilings, and returns no partial successful aggregate on failure. It deliberately does not choose guest bases or map dependency ELF images.
- The reproducible real ARMv7/Android fixture remains generated with pinned NDK r27d / API 26 inputs. It has four `PT_LOAD` segments with `p_align=0x4000`, BSS, one `PT_DYNAMIC`, and observed SONAME/REL/SYMTAB/STRTAB/GNU_HASH-related tags with no `DT_NEEDED`; the linker-string integration validates SONAME `liba32android_loader_fixture.so` with an explicit 64-byte ceiling and zero NEEDED names.
- Repository workflow state uses root `AGENTS.md`, durable `.agent/` files, and `specs/<id>-<feature>/{requirements,design,tasks}.md` for feature-scale work. `specs/000-current-baseline/` converts the already implemented work through PR #11 into that structure.

## Partial / not implemented

- Dependency guest mapping/loading beyond bounded image acquisition, recursive graph/link-map/cycle/dedup semantics, Android search-path/namespace/pathname policy, and full dynamic symbol-table semantics: NOT IMPLEMENTED.
- ARM relocations: NOT IMPLEMENTED.
- Symbol lookup/interposition: NOT IMPLEMENTED.
- RELRO/TLS processing: NOT IMPLEMENTED.
- Automatic `ET_DYN` guest-VA allocation: NOT IMPLEMENTED.
- End-to-end execution of the real ARM32 fixture through the runtime on Android: NOT IMPLEMENTED / NOT RUN.
- Actual 16 KiB Android host-page behavior: NOT RUN.
- Broader Android/vendor/kernel compatibility for the high-base reservation: PARTIAL evidence only.

## Validation

### Opt-in crash-test diagnostics

PR #23 is merged to `bleeding` as `709e9ce74e58ee12d925b64c8466b9afa58cc4a6`. Exact-head GitHub Actions run `35436087356` (#134) at `4cb17e059b8258f0cf8ce462c58012ab7d55249d` is PASS: Linux A32 smoke PASS and Android `arm64-v8a` cross-build PASS. CI verified the opt-in `--crash-test` paths and markers statically and did not execute the destructive mode. No post-merge workflow run was observed for the squash-merge commit during reconciliation. Real Android crash-marker/tombstone coexistence is NOT RUN.

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

No implementation blocker remains for merged dependency acquisition, probe metadata correction, focused CPU regressions, or opt-in crash-test diagnostics. The remaining highest-value gaps are device evidence: crash-marker/tombstone coexistence and actual 16 KiB Android host-page behavior.

## Important temporary facts
- Diagnostic tools have merged opt-in `--crash-test` paths that require successful handler setup, emit armed/SIGABRT markers, then call `abort()`; ordinary smoke/probe execution never selects them. Exact-head CI #134 PASS; real-device tombstone coexistence remains NOT RUN.
- CPU regression slice adds Thumb branch/call/memory/stack, Thumb SVC exception, instruction-fetch fault, and Thumb data-fault coverage; nearby test comments/setup are cleaned without changing runtime contracts. Exact-head CI is NOT RUN.

- `specs/002-elf32-linker-strings/` is merged through PR #15 as `dbc329e2205828c97267a2de60ce0771c4173cb6`. T001 PASS #97, T002 PASS #99, T003 PASS #102, T005 feature gate PASS #106, and persistence-only closeout PASS #108.
- Dependency loading/search-path policy remains deliberately deferred by merged `002`; `003-elf32-dependency-resolution` is now readiness-checked on `m4-elf32-dependency-resolution`.
- `003` assigns filesystem/search-path/namespace lookup policy to an injected provider, preserves one request/result occurrence per ordered `DT_NEEDED`, requires explicit dependency-count/per-image/total-image byte limits, and stops before guest mapping because `ET_DYN` placement remains explicit.
- `003-elf32-dependency-resolution` is merged through PR #17 as `c1f0f30d6fde7c73c93dec83f5808353a838c856`: T001 PASS #112, T002 PASS #116, T003 PASS #118, T004 DONE, T005 PASS #122.

- `specs/000-current-baseline/` is a documentation conversion of already implemented behavior; the runtime evidence above remains its validation basis.
- User-owned WSL local validation is now available: pinned NDK r27d (`27.3.13750724`) fixture/host build PASS, 26/26 CTest PASS, and Android `arm64-v8a` cross-build PASS were reported on 2026-09-19. The pasted local logs did not include a Git commit identity, so this is environment-capability evidence rather than an exact-commit release gate.
- Real Android/AArch64 runtime behavior still requires Termux/device execution; the user prefers downloading the CI-produced runtime-smoke artifact for those runs.
- The next feature-scale implementation must get a new `specs/<id>-<feature>/` requirements/design/tasks chain instead of extending `specs/000-current-baseline/`.
