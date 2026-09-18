# Tasks — ELF32 Linker Metadata

## T001 — Add linker-metadata API and semantic collection
- Requirements: R1-R4, R8-R9; AC1-AC4, AC8
- Depends on: none
- Scope: add `src/elf/elf32_linker_metadata.{h,cpp}`; classify the supported tag set; collect singleton values and ordered `DT_NEEDED`; reject duplicate supported singletons and incomplete tag groups; keep deferred/unknown tags tolerated.
- Validation: focused synthetic metadata test for valid collection, duplicate policy, missing companions, repeated NEEDED order, and deferred-tag tolerance.
- Status: DONE — CI #78 PASS

## T002 — Add rebasing and guest-range validation
- Requirements: R2, R5-R7, R9; AC2, AC5-AC7
- Depends on: T001
- Scope: checked load-bias addition for STRTAB/SYMTAB/REL; checked range ends; bounded-read validation through `GuestMemory`; enforce `DT_SYMENT == 16`, `DT_RELENT == 8`, RELSZ divisibility, and string-offset bounds.
- Validation: synthetic overflow, unreadable-range, bad-entry-size, bad-RELSZ, and bad-string-offset regressions; verify guest bytes remain unchanged after failures.
- Status: IMPLEMENTED — CI NOT RUN

## T003 — Integrate the reproducible real ARM32 fixture
- Requirements: R10; AC9
- Depends on: T001, T002
- Scope: load the real fixture, structurally parse its dynamic array, build linker metadata with the loader's load bias, and assert valid STRTAB/SYMTAB/REL/SONAME descriptors plus no NEEDED entries.
- Validation: real-fixture linker-metadata integration test in CTest.
- Status: TODO

## T004 — Converge architecture and durable state
- Requirements: R1, R8, R10; AC10
- Depends on: T001-T003
- Scope: document the new linker-metadata boundary, keep deferred linker semantics explicit, update `.agent/STATE.md` / `.agent/NEXT.md`, and ensure README wording remains accurate.
- Validation: consistency pass across requirements/design/tasks/code/tests/docs/state.
- Status: TODO

## T005 — CI gate
- Requirements: R10; AC10
- Depends on: T001-T004
- Scope: run the repository's existing GitHub Actions workflow on the feature head.
- Validation:
  - existing baseline tests remain PASS;
  - new linker-metadata synthetic/integration tests PASS;
  - reproducible ARM32 fixture generation remains PASS;
  - exact `liba32android.so` output-name check remains PASS;
  - Android `arm64-v8a` cross-build and diagnostics remain PASS.
- Status: TODO

## Readiness Check

- [x] Every acceptance criterion has an implementation/validation path.
- [x] Every task traces to a requirement/design need.
- [x] No unresolved question blocks safe implementation.
- [x] Design preserves D-0003/D-0004 and repository ELF/memory boundaries.
- [x] No migration or public API compatibility break is required.
- [x] Security-relevant guest pointer/range validation is explicit.
- [x] Task dependencies are acyclic and executable.
- [x] The first implementation slice can be completed independently as T001 before opening the range-validation/fixture fronts.
