# Tasks — ELF32 PLT REL Metadata

Status: ACTIVE — T001 implementation CI #221 PASSed; AC8 real-fixture absence oracle added and revalidation pending

Base revision: `ef5f08eb6d184c12b252ac6f59994e8efed21d5b`

## T001 — Validate and expose PLT REL metadata

- Status: ACTIVE — implementation exact-head CI #221 / run `35933289817` PASSed at `47070bef73acd14464137789585abd4972878ab4`; convergence review found AC8 lacked an explicit real-fixture no-PLT assertion, which is now prepared and requires exact-head revalidation
- Requirements: R1-R9; AC1-AC8
- Depends on: none
- Scope:
  - extend `src/elf/elf32_linker_metadata.{h,cpp}` with additive PLT REL descriptors/errors;
  - recognize `DT_PLTRELSZ`, `DT_PLTREL`, and `DT_JMPREL`;
  - require a complete singleton group and `DT_PLTREL == DT_REL`;
  - validate 8-byte divisibility, checked rebasing, range fit, and readability;
  - extend `tests/elf32_linker_metadata.cpp` with focused positive/malformed cases.
- Non-goals: no REL entry decode, JUMP_SLOT, lazy binding, PLTGOT semantics, or guest writes.
- Validation: focused linker-metadata CTest plus neighboring ELF/linker tests and both Android CI jobs.
- Stop condition: validated PLT descriptor exists and T001 tests pass; do not begin JUMP_SLOT work.

## T002 — Converge docs/state and final exact-head gate

- Status: QUEUED
- Requirements: R8-R9; AC7-AC9
- Depends on: T001 PASS
- Scope: update architecture docs/current phase/spec state, reconcile semantics, and verify final exact-head CI.
- Validation: Linux CTest PASS, Android x86_64 probe PASS, Android arm64-v8a cross-build PASS, docs/spec/state consistent.

## Readiness check

PASS: acceptance and negative cases are explicit; main-REL semantics remain unchanged; PLT metadata remains read-only and guest-only; dependencies are acyclic; T001 fits one bounded round; GitHub source/CI capabilities are available.

Exact next action: commit the AC8 real-fixture assertion, re-run exact-head Linux CTest + Android x86_64 + Android arm64-v8a CI, and mark T001 PASS only if all three jobs PASS.
