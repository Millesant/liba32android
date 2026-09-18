# Current State

Last updated: 2026-09-18
Current phase: M4 linker-string slice implemented through T004; final exact-head CI gate pending
Integration branch: `bleeding`
Last merged runtime PR: #13
Runtime baseline commit: `1169f4eff1fb4ba35a74f55167b3904f12ff2425`
Active runtime work: `002-elf32-linker-strings` T005 final exact-head CI gate on `m4-elf32-dynamic-strings`

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
- The reproducible real ARMv7/Android fixture remains generated with pinned NDK r27d / API 26 inputs. It has four `PT_LOAD` segments with `p_align=0x4000`, BSS, one `PT_DYNAMIC`, and observed SONAME/REL/SYMTAB/STRTAB/GNU_HASH-related tags with no `DT_NEEDED`; the linker-string integration validates SONAME `liba32android_loader_fixture.so` with an explicit 64-byte ceiling and zero NEEDED names.
- Repository workflow state uses root `AGENTS.md`, durable `.agent/` files, and `specs/<id>-<feature>/{requirements,design,tasks}.md` for feature-scale work. `specs/000-current-baseline/` converts the already implemented work through PR #11 into that structure.

## Partial / not implemented

- `DT_NEEDED` dependency loading, search-path/namespace policy, pathname semantics, and full dynamic symbol-table semantics: NOT IMPLEMENTED.
- `DT_NEEDED` dependency loading: NOT IMPLEMENTED.
- ARM relocations: NOT IMPLEMENTED.
- Symbol lookup/interposition: NOT IMPLEMENTED.
- RELRO/TLS processing: NOT IMPLEMENTED.
- Automatic `ET_DYN` guest-VA allocation: NOT IMPLEMENTED.
- End-to-end execution of the real ARM32 fixture through the runtime on Android: NOT IMPLEMENTED / NOT RUN.
- Actual 16 KiB Android host-page behavior: NOT RUN.
- Broader Android/vendor/kernel compatibility for the high-base reservation: PARTIAL evidence only.

## Validation

### M4 linker-string feature

T001 bounded STRTAB reading is PASS on GitHub Actions run `35387506251` (#97). T002 SONAME/NEEDED aggregation is PASS on run `35391818125` (#99). T003 real-fixture string integration is PASS on run `35402559596` (#102): Linux CTest executed `elf32_real_linker_strings` and reported 24/24 tests PASS, and Android arm64-v8a cross-build PASS. T004 architecture/README/state convergence is complete. The latest convergence head still requires the final T005 exact-head CI gate.

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

No blocker prevents the final CI gate for `002-elf32-linker-strings`. Actual 16 KiB Android host-page behavior remains an independent evidence gap rather than a blocker for this linker work.

## Important temporary facts

- `specs/002-elf32-linker-strings/` is implemented through T004. T001 is PASS on #97, T002 on #99, and T003 on #102. T005 final exact-head CI is NOT RUN on the documentation/state-converged head.
- Dependency loading/search-path policy remains deliberately deferred; this feature only materializes bounded byte strings.

- `specs/000-current-baseline/` is a documentation conversion of already implemented behavior; the runtime evidence above remains its validation basis.
- The next feature-scale implementation must get a new `specs/<id>-<feature>/` requirements/design/tasks chain instead of extending `specs/000-current-baseline/`.
