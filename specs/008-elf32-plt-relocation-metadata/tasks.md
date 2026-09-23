# Tasks — ELF32 PLT REL Metadata

Status: ACTIVE — T001 VERIFIED; T002 convergence/final gate active

Base revision: `ef5f08eb6d184c12b252ac6f59994e8efed21d5b`

## T001 — Validate and expose PLT REL metadata

- Status: VERIFIED — CI #222 / run `35933694619` at `9a81ed71a027beb166970bcf137bac9a71112f98` completed all required Linux, Android x86_64, and Android arm64-v8a jobs successfully; the head includes the explicit real-fixture no-PLT assertion required by AC8
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

- Status: ACTIVE — documentation/spec/state convergence prepared; final exact-head CI pending
- Requirements: R8-R9; AC7-AC9
- Depends on: T001 PASS
- Scope: update architecture docs/current phase/spec state, reconcile semantics, and verify final exact-head CI.
- Validation: Linux CTest PASS, Android x86_64 probe PASS, Android arm64-v8a cross-build PASS, docs/spec/state consistent.

## Readiness check

PASS: acceptance and negative cases are explicit; main-REL semantics remain unchanged; PLT metadata remains read-only and guest-only; dependencies are acyclic; T001 fits one bounded round; GitHub source/CI capabilities are available.

Exact next action: commit T002 documentation/spec/state convergence on `bleeding`, then verify the final exact-head Linux CTest + Android x86_64 + Android arm64-v8a CI gate. On PASS, persist feature-008 DONE state without broadening into JUMP_SLOT work.
