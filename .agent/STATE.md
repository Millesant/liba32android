# Current State

Last updated: 2026-09-16
Current milestone: M3 ELF32 loading
Integration branch: `bleeding`
Last merged PR: #8
Merged integration commit: `8059c07b989f845b5b48e306e39cdca6c66d502b`

## Working / proven

- M2 guest address space is COMPLETE for its current scope: logical 32-bit guest VAs, `MappedGuestMemory`, high-base 4 GiB reservation, map/protect/unmap lifecycle, Dynarmic fastmem and callback fallback are implemented and proven on Linux plus the known Android 16 / SDK 36 AArch64 Termux environment.
- D-0003 remains accepted: guest VAs are independent from host pointer identity.
- D-0004 remains accepted: high-base contiguous fastmem is the preferred first Android acceleration path; callbacks remain the correctness fallback.
- The shared runtime produces exactly `liba32android.so`.
- First M3 ELF32 mapping slice is IMPLEMENTED and merged:
  - engine-independent `src/elf/elf32_loader.*` API;
  - ELF32/little-endian/current-version/`EM_ARM` validation;
  - `ET_DYN` with explicit page-aligned guest base and computed load bias;
  - fixed-address `ET_EXEC` loading;
  - program-header/file/address/alignment/permission validation before guest mutation;
  - `PT_LOAD` map/copy/BSS zero-fill/final-protection lifecycle;
  - rollback of loader-owned mappings after map/write/protect failure;
  - guest-only result metadata; no host pointers in loader results.
- The first loader slice deliberately rejects page-overlapping `PT_LOAD` mappings and RWX segments rather than silently broadening permissions.
- Loader architecture and current limitations are documented in `docs/architecture/elf32-loader.md`.

## Validation

### Final PR head

GitHub Actions run `35085844432` (#51) on PR #8 head `a48d4459192aa7dbe48600d61d121ddb4ed542a0`: PASS.

### Post-merge `bleeding`

GitHub Actions run `35086133758` (#52) on merge commit `8059c07b989f845b5b48e306e39cdca6c66d502b`: PASS.

- Linux configure/build: PASS
- Linux shared-library filename check: PASS
- Linux CTest: 16/16 PASS
  - existing 10 CPU/memory/fastmem tests: PASS
  - `elf32_valid_dynamic_load`: PASS
  - `elf32_valid_exec_load`: PASS
  - `elf32_header_validation`: PASS
  - `elf32_program_header_bounds`: PASS
  - `elf32_segment_validation`: PASS
  - `elf32_address_conflict`: PASS
- Android arm64-v8a runtime + diagnostics configure/build/link: PASS
- Android shared-library/diagnostic checks: PASS
- Android runtime/probe/runtime-smoke artifact uploads: PASS

Local clone/build in this chat environment: BLOCKED because the container could not resolve `github.com`; GitHub Actions was the executed validation environment for this round.

## Evidence boundary

The first M3 loader tests use synthetic ELF32 images and prove parser/mapping behavior on Linux. Android CI proves the same loader source compiles/links into the arm64 runtime, but there is not yet a real ARM32 ELF fixture integration test or separate Android device ELF-loader smoke.

The first slice does NOT implement or claim:

- `PT_DYNAMIC` processing;
- DT_NEEDED dependency loading;
- dynamic symbols/string tables;
- ARM relocations;
- symbol lookup/interposition;
- RELRO/TLS handling;
- automatic guest-VA allocation;
- real Android ARM32 shared-library execution.

## Current blocker

No blocker prevents continuing M3. The next evidence gap is a reproducible real ARM32 ELF fixture. Its observed `PT_LOAD` layout should determine whether the current deliberate page-overlap rejection needs a safe shared-page implementation.
