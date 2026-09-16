# Current State

Last updated: 2026-09-16
Current milestone: M3 ELF32 loading
Integration branch: `bleeding`
Active work: PR #8 (`m3-elf32-loader`)

## Working / proven

- M2 guest address space is COMPLETE for its current scope: logical 32-bit guest VAs, `MappedGuestMemory`, high-base 4 GiB reservation, map/protect/unmap lifecycle, Dynarmic fastmem and callback fallback are implemented and proven on Linux plus the known Android 16 / SDK 36 AArch64 Termux environment.
- D-0003 remains accepted: guest VAs are independent from host pointer identity.
- D-0004 remains accepted: high-base contiguous fastmem is the preferred first Android acceleration path; callbacks remain the correctness fallback.
- The shared runtime produces exactly `liba32android.so`.
- First M3 ELF32 mapping slice is IMPLEMENTED on PR #8:
  - engine-independent `src/elf/elf32_loader.*` API;
  - ELF32/little-endian/current-version/`EM_ARM` validation;
  - `ET_DYN` with explicit page-aligned guest base and computed load bias;
  - fixed-address `ET_EXEC` loading;
  - program-header/file/address/alignment/permission validation before guest mutation;
  - `PT_LOAD` map/copy/BSS zero-fill/final-protection lifecycle;
  - rollback of loader-owned mappings after map/write/protect failure;
  - guest-only result metadata; no host pointers in loader results.
- The first loader slice deliberately rejects page-overlapping `PT_LOAD` mappings and RWX segments rather than silently broadening permissions.

## Validation

### PR #8 implementation checkpoint

GitHub Actions run `35085429455` (#48) on head `1cebdcb631157c4fc530dfa9b5b556b0e6d7a1b9`: PASS.

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

Local clone/build in this chat environment: BLOCKED because the container could not resolve `github.com`; GitHub Actions is the executed validation environment for this round.

## Evidence boundary

The new M3 tests use synthetic ELF32 images and prove parser/mapping behavior on the Linux host implementation. Android CI proves the same loader source compiles/links into the arm64 runtime, but there is not yet a separate real-device ELF32-loader smoke.

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

No blocker prevents integrating the first M3 loader slice. The current loader limitation most likely to matter for real binaries is deliberate rejection of page-overlapping `PT_LOAD` segments; it should be changed only with a concrete fixture/evidence and without silently broadening permissions.
