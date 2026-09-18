# Tasks — Current Runtime Baseline

This is a conversion of completed implementation work into the current dependency-ordered task format. Status reflects repository state through PR #11; detailed chronology remains in Git history and existing evidence documents.

## T001 — Establish A32 CPU adapter
- Requirements: R1, R6
- Depends on: none
- Scope: select/pin Dynarmic, keep it behind `src/cpu/`, prove ARM and Thumb execution plus core register/control-flow behavior.
- Validation: host CPU smoke/execution tests; Android `arm64-v8a` cross-build.
- Status: DONE

## T002 — Establish generic guest-memory seam
- Requirements: R2, R3, R6
- Depends on: T001
- Scope: introduce `memory::GuestMemory`, bounded linear implementation, explicit access-failure behavior, and remove CPU dependence on a hard-coded test buffer.
- Validation: guest-memory bounds and CPU memory/stack tests.
- Status: DONE

## T003 — Add mapped 4 GiB guest address space and fastmem path
- Requirements: R2, R3, R7
- Depends on: T002
- Scope: `MappedGuestMemory`, logical 32-bit VA mapping/protection lifecycle, high-base contiguous reservation, internal fastmem-base capability, Dynarmic fastmem with callback fallback.
- Validation: mapped-memory lifecycle test, fastmem/fallback test, recorded Android/AArch64 runtime probe evidence.
- Status: DONE

## T004 — Add validated ELF32 PT_LOAD mapping
- Requirements: R4, R6, R7
- Depends on: T003
- Scope: ELF32/ARM validation; `ET_EXEC` and explicit-base `ET_DYN`; `PT_LOAD` planning/mapping/copy/BSS/final permissions; alignment/overflow/conflict checks; rollback for loader-owned mutation failures.
- Validation: synthetic ELF loader tests plus reproducible real Android ARM32 fixture integration.
- Status: DONE

## T005 — Add validated PT_DYNAMIC guest metadata
- Requirements: R4, R5, R6
- Depends on: T004
- Scope: discover zero-or-one `PT_DYNAMIC`, validate file/address/readability/containment/file-to-load mapping, return guest address and sizes without parsing/linking.
- Validation: synthetic positive/negative metadata cases and exact real-fixture metadata checks.
- Status: DONE

## T006 — Parse Elf32_Dyn structurally
- Requirements: R5, R6, R7
- Depends on: T005
- Scope: read 8-byte entries from guest memory, preserve signed raw tags/raw values, retain first `DT_NULL`, ignore following padding, reject malformed ranges/unreadable bytes/unterminated arrays, do not begin linking.
- Validation: synthetic dynamic-array test and real ARM32 fixture dynamic-array integration test.
- Status: DONE

## T007 — Converge baseline evidence and boundaries
- Requirements: R6, R7; AC1-AC8
- Depends on: T001-T006
- Scope: keep architecture/evidence docs explicit about implemented behavior, inference, and NOT RUN gaps; ensure dynamic linking/application compatibility are not claimed.
- Validation: GitHub Actions run `35204765081` on integration commit `ac008b2d2a3158ffa4cb285e88cfabacea2ca4a2` completed successfully with 20/20 CTest cases and Android `arm64-v8a` cross-build/diagnostic artifact production.
- Status: DONE

## Baseline convergence

All completed tasks above trace to `requirements.md` and `design.md`. Subsequent feature-scale implementation must receive its own package rather than appending future tasks to this historical baseline.
