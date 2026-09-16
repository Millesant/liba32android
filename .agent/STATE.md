# Current State

Last updated: 2026-09-16
Current milestone: M3 ELF32 loading
Integration branch: `bleeding`
Last merged PR: #8
Merged integration commit: `8059c07b989f845b5b48e306e39cdca6c66d502b`
Active work: PR #9 (`m3-real-arm32-fixture`)

## Working / proven

- M2 guest address space is COMPLETE for its current scope: logical 32-bit guest VAs, `MappedGuestMemory`, high-base 4 GiB reservation, map/protect/unmap lifecycle, Dynarmic fastmem and callback fallback are implemented and proven on Linux plus the known Android 16 / SDK 36 AArch64 Termux environment.
- D-0003 remains accepted: guest VAs are independent from host pointer identity.
- D-0004 remains accepted: high-base contiguous fastmem is the preferred first Android acceleration path; callbacks remain the correctness fallback.
- The shared runtime produces exactly `liba32android.so`.
- First M3 ELF32 mapping slice is IMPLEMENTED and merged:
  - engine-independent `src/elf/elf32_loader.*` API;
  - ELF32/little-endian/current-version/`EM_ARM` validation;
  - `ET_DYN` with explicit guest base and computed load bias;
  - fixed-address `ET_EXEC` loading;
  - program-header/file/address/alignment/permission validation before guest mutation;
  - `PT_LOAD` map/copy/BSS zero-fill/final-protection lifecycle;
  - rollback of loader-owned mappings after map/write/protect failure;
  - guest-only result metadata; no host pointers in loader results.
- PR #9 adds a reproducible real ARMv7/Android ELF fixture generated from source with the project-pinned NDK r27d / API 26 toolchain.
- CI builds the real fixture twice and requires byte-identical output before loader testing.
- The observed real fixture is ARM ELF32 `ET_DYN`, contains four `PT_LOAD` segments, `PT_DYNAMIC`, executable and writable content, and BSS. All four PT_LOAD segments have `p_align=0x4000` (16 KiB).
- Real-fixture evidence exposed and PR #9 fixes an ET_DYN alignment requirement: load bias must preserve each PT_LOAD `p_align`, not merely the current host page size.
- The real fixture loader test validates exact PT_LOAD file bytes, BSS zero-fill, mapping metadata, final permissions, aligned load bias, and rejection of a 4 KiB-aligned load bias that violates the real fixture's 16 KiB `p_align`.
- The current loader still deliberately rejects page-overlapping `PT_LOAD` mappings and RWX segments rather than silently broadening permissions. The current real fixture does not demonstrate a need for shared-page handling.
- Loader architecture and real-fixture evidence are documented in `docs/architecture/elf32-loader.md` and `docs/research/evidence/arm32-loader-fixture-ndk-r27d-2026-09-16.md`.

## Validation

### PR #9 current implementation checkpoint

GitHub Actions run `35109819830` (#58) on PR #9 head `c517867fb355c22ae0ee2992dd0771d711dc2ceb`: PASS.

Linux:

- pinned NDK availability: PASS
- real ARM32 fixture generation: PASS
- repeated byte-identical fixture generation: PASS
- fixture SHA-256: `6c2dbda2dec94eaa022ad09391ed3988c3a41828e5c1c065fc0101e5725b84c2`
- configure/build: PASS
- shared-library filename check: PASS
- CTest: 17/17 PASS
- `elf32_real_arm32_fixture`: PASS
- real fixture `max_p_align=16384`: observed
- host page size: observed 4096
- misaligned ET_DYN load bias rejection: PASS / `fixture.misaligned_bias_rejected=true`
- fixture load with `load_bias=0x02000000`: PASS
- loader evidence artifact upload: PASS
- artifact ID: `10452280814`
- artifact ZIP digest: `sha256:45011484cca2274066b96389979c68261339ffebdb6b579a44692af10fd19e56`

Android arm64-v8a:

- runtime + diagnostics configure/build/link: PASS
- shared-library/diagnostic checks: PASS
- Android runtime/probe/runtime-smoke artifact uploads: PASS

Run #55 (`35109127963`) was an earlier implementation checkpoint and also passed 17/17 plus Android cross-build. Run #58 supersedes it as the current code checkpoint because it includes the PT_LOAD load-bias alignment fix.

## Observed real fixture layout

All PT_LOAD entries use `p_align=0x4000`:

1. `offset=0x000000`, `vaddr=0x00000000`, `filesz=0x224`, `memsz=0x224`, flags R.
2. `offset=0x000224`, `vaddr=0x00004224`, `filesz=0x48`, `memsz=0x48`, flags RX.
3. `offset=0x00026c`, `vaddr=0x0000826c`, `filesz=0x68`, `memsz=0x0d94`, flags RW.
4. `offset=0x0002d4`, `vaddr=0x0000c2d4`, `filesz=0x4`, `memsz=0x8`, flags RW.

`PT_DYNAMIC` is present at guest virtual address `0x826c` before load bias. Dynamic entries observed include REL/SYMTAB/STRTAB/GNU_HASH metadata, but the loader does not interpret or apply them yet.

## Evidence boundary

Observed on the Linux CI host:

- real Android-toolchain ARM32 ELF generation;
- deterministic fixture bytes for the pinned toolchain;
- 16 KiB PT_LOAD alignment metadata;
- real-fixture PT_LOAD mapping/copy/BSS/permissions;
- preservation of PT_LOAD p_align through load bias.

Inferred from the real program headers:

- the four PT_LOAD ranges occupy distinct 16 KiB windows, so this fixture would not require shared-page PT_LOAD handling solely because of 16 KiB page granularity.

NOT RUN / not demonstrated:

- `MappedGuestMemory` on an actual 16 KiB Android kernel/page configuration;
- loading this fixture through the runtime on a real Android device;
- ARM relocations;
- dynamic symbol/string table consumption;
- DT_NEEDED resolution;
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
- reproducible real ARM32 Android ELF loader-only integration coverage.

Not implemented:

- PT_DYNAMIC discovery as loader result metadata;
- dynamic linking / DT_NEEDED;
- symbol resolution;
- ARM relocations;
- RELRO/TLS;
- automatic ET_DYN guest-VA allocation;
- real Android ARM32 library execution.

## Current blocker

No blocker prevents continuing M3. Actual 16 KiB Android host-page behavior remains an evidence gap, but it does not block the next metadata-only loader slice.
