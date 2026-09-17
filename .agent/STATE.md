# Current State

Last updated: 2026-09-17
Current milestone: M3 ELF32 loading
Integration branch: `bleeding`
Last merged PR: #10
Merged integration commit: `d3ff97566c30417eb7bc37f6dd8aed76751ce070`
Active work: none

## Working / proven

- M2 guest address space is COMPLETE for its current scope: logical 32-bit guest VAs, `MappedGuestMemory`, high-base 4 GiB reservation, map/protect/unmap lifecycle, Dynarmic fastmem and callback fallback are implemented and proven on Linux plus the known Android 16 / SDK 36 AArch64 Termux environment.
- D-0003 remains accepted: guest VAs are independent from host pointer identity.
- D-0004 remains accepted: high-base contiguous fastmem is the preferred first Android acceleration path; callbacks remain the correctness fallback.
- The shared runtime produces exactly `liba32android.so`.
- M3 ELF32 mapping is implemented through validated `PT_LOAD` mapping plus guest-only `PT_DYNAMIC` range discovery:
  - engine-independent `src/elf/elf32_loader.*` API;
  - ELF32/little-endian/current-version/`EM_ARM` validation;
  - `ET_DYN` with explicit guest base and load bias preserving each `PT_LOAD p_align`;
  - fixed-address `ET_EXEC` loading;
  - `PT_LOAD` map/copy/BSS zero-fill/final-protection lifecycle with rollback of loader-owned mappings after mutation failures;
  - guest-only result metadata; no host pointers in loader results;
  - missing `PT_DYNAMIC` is valid;
  - a present `PT_DYNAMIC` must be unique, non-empty, address/file-bounds valid, inside a readable `PT_LOAD`, and its non-empty file-backed range must match the file bytes that the containing `PT_LOAD` maps at the same guest VA;
  - malformed `PT_DYNAMIC` metadata is rejected before guest mappings are created;
  - successful `PT_DYNAMIC` discovery reports biased guest VA, `p_filesz`, and `p_memsz` only; it does not parse dynamic tags or perform linking.
- PR #10 is merged. Synthetic coverage includes valid/missing PT_DYNAMIC plus multiple, empty, file-size, file-bounds, address-overflow, outside-load, non-readable, and file-to-load-mismatch rejection cases.
- The reproducible real ARMv7/Android fixture remains generated from source with the pinned NDK r27d / API 26 toolchain. It is ARM ELF32 `ET_DYN`, has four `PT_LOAD` segments with `p_align=0x4000`, executable/writable content, BSS, and one `PT_DYNAMIC` at pre-bias guest VA `0x826c` with file/memory size `0x60`.
- The real-fixture integration test validates exact PT_LOAD bytes/BSS/permissions/alignment plus exact biased PT_DYNAMIC result metadata. The loader also validates that the real PT_DYNAMIC file range is the range exposed through its containing PT_LOAD.
- The loader still deliberately rejects page-overlapping `PT_LOAD` mappings and RWX segments rather than silently broadening permissions. The current real fixture does not demonstrate a need for shared-page handling.
- Loader architecture and real-fixture evidence are documented in `docs/architecture/elf32-loader.md` and `docs/research/evidence/arm32-loader-fixture-ndk-r27d-2026-09-16.md`.

## Validation

### Final PR #10 head

GitHub Actions run `35203342934` (#69) on PR #10 head `a5ebd54e9391f2f9aa0d365cd0d10c67fa0fbfef`: PASS.

### Post-merge `bleeding`

GitHub Actions run `35203647032` (#70) on merge commit `d3ff97566c30417eb7bc37f6dd8aed76751ce070`: PASS.

Linux:

- pinned NDK availability: PASS
- real ARM32 fixture generation: PASS
- repeated byte-identical fixture generation: PASS
- configure/build: PASS
- shared-library filename check: PASS
- CTest suite (18 registered tests): PASS
- PT_DYNAMIC synthetic validation/discovery coverage: PASS through the CTest suite
- real ARM32 fixture loader evidence generation: PASS
- real fixture PT_DYNAMIC exact guest metadata and PT_LOAD file-mapping validation: PASS
- host page size for this evidence remains 4096; actual 16 KiB host behavior remains NOT RUN
- post-merge fixture artifact ID: `10488882816`
- post-merge fixture artifact digest: `sha256:b5752dc62892c2ddfbd939e094ca6719b0947a9b06beeb2edacf84a2ea4c2c40`

Android arm64-v8a:

- runtime + diagnostics configure/build/link: PASS
- shared-library/diagnostic checks: PASS
- Android runtime/probe/runtime-smoke artifact uploads: PASS
- runtime artifact ID: `10488569129`
- address-space probe artifact ID: `10488673575`
- runtime-smoke artifact ID: `10489321109`

## Observed real fixture layout

All PT_LOAD entries use `p_align=0x4000`:

1. `offset=0x000000`, `vaddr=0x00000000`, `filesz=0x224`, `memsz=0x224`, flags R.
2. `offset=0x000224`, `vaddr=0x00004224`, `filesz=0x48`, `memsz=0x48`, flags RX.
3. `offset=0x00026c`, `vaddr=0x0000826c`, `filesz=0x68`, `memsz=0x0d94`, flags RW.
4. `offset=0x0002d4`, `vaddr=0x0000c2d4`, `filesz=0x4`, `memsz=0x8`, flags RW.

`PT_DYNAMIC` is present at pre-bias guest VA `0x826c`, `p_offset=0x26c`, and `p_filesz=p_memsz=0x60`. Dynamic entries observed in fixture evidence include REL/SYMTAB/STRTAB/GNU_HASH metadata, but the loader does not structurally parse or interpret the `Elf32_Dyn` array yet.

## Evidence boundary

Observed on Linux CI:

- real Android-toolchain ARM32 ELF generation;
- deterministic fixture generation for the pinned toolchain;
- 16 KiB PT_LOAD alignment metadata;
- real-fixture PT_LOAD mapping/copy/BSS/permissions;
- preservation of PT_LOAD p_align through load bias;
- PT_DYNAMIC guest-range discovery and exact biased metadata;
- PT_DYNAMIC containment/readability/file-to-PT_LOAD mapping validation before guest mutation.

Inferred from the real program headers:

- the four PT_LOAD ranges occupy distinct 16 KiB windows, so this fixture would not require shared-page PT_LOAD handling solely because of 16 KiB page granularity.

NOT RUN / not demonstrated:

- `MappedGuestMemory` on an actual 16 KiB Android kernel/page configuration;
- loading this fixture through the runtime on a real Android device;
- structural parsing/validation of the loaded `Elf32_Dyn` entry array;
- dynamic symbol/string table consumption;
- DT_NEEDED resolution;
- ARM relocations;
- RELRO enforcement;
- execution of the loaded ARM32 fixture function.

## Milestone status

M3 ELF32 loading remains PARTIAL.

Implemented/tested within M3:

- ELF32/ARM validation;
- ET_EXEC fixed loading;
- ET_DYN explicit-base loading;
- PT_LOAD mapping, copy, BSS and final permissions;
- overflow/alignment/conflict checks;
- PT_LOAD p_align-preserving load bias;
- synthetic malformed coverage;
- reproducible real ARM32 Android ELF loader-only integration coverage;
- validated PT_DYNAMIC guest-range discovery without dynamic linking.

Not implemented:

- structural `Elf32_Dyn` array parsing/tag metadata;
- dynamic linking / DT_NEEDED;
- symbol resolution;
- ARM relocations;
- RELRO/TLS;
- automatic ET_DYN guest-VA allocation;
- real Android ARM32 library execution.

## Current blocker

No blocker prevents continuing M3. Actual 16 KiB Android host-page behavior remains an evidence gap, but it does not block the next metadata-only dynamic-array slice.
