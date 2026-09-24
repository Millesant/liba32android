# Tasks — ELF32 GNU RELRO protection

Status: ACTIVE — T001/T002 VERIFIED; T003 real post-relocation sealing implementation prepared, exact-head validation NOT RUN

Base revision: `fd6670f3d40d51ad6fa5995b27ede87e4aa1b51c`
Control revision: `9240ab19507b86491398a5c9fdf0deb58e2fdc91`

## T001 — Validate and expose PT_GNU_RELRO metadata

- Status: VERIFIED — exact-head CI #231 / run `35942933233` PASSed at `e3ea30a9445c86546933d008ee5a336bd9e91e8e`; Linux passed 47/47 CTest including `elf32_relro_metadata`, Android x86_64 and Android arm64-v8a also PASSed
- Requirements: R1-R4, R11, R13-R14; AC1-AC4, AC9-AC10
- Scope:
  - recognize `PT_GNU_RELRO` in the shared load plan;
  - validate non-empty/overflow/page-rounded readable PT_LOAD coverage;
  - expose exact + page-rounded load-biased metadata from `Elf32LoadResult`;
  - preserve original PT_LOAD permissions;
  - extend synthetic load-plan/loader tests and the pinned real fixture oracle.
- Validation:
  - focused synthetic loader cases;
  - real fixture reports GNU RELRO metadata matching raw program headers;
  - existing loader/linker/relocation tests PASS;
  - exact-head Linux + Android CI PASS.

## T002 — Bounded transactional RELRO sealing

- Status: VERIFIED — exact-head CI #232 / run `35943552213` PASSed at `655f933f46c4bb28e5c36fe34b628b92af1f679b`; Linux passed 48/48 CTest including `elf32_relro_seal`, Android x86_64 and Android arm64-v8a also PASSed
- Depends on: T001
- Requirements: R5-R10, R13-R14; AC5-AC9
- Scope:
  - add `elf32_relro.{h,cpp}`;
  - caller-selected page bound;
  - complete page/mapping/permission preflight;
  - overlap deduplication and idempotence;
  - RW -> R sealing only;
  - reverse permission rollback on later protect failure.
- Validation:
  - synthetic success/bounds/malformed/unmapped/unreadable/executable/overlap/idempotence cases;
  - neighboring relocation tests remain green;
  - exact-head Linux + Android CI PASS.

## T003 — Real post-relocation GNU RELRO sealing

- Status: IMPLEMENTED — exact-head validation NOT RUN
- Depends on: T002
- Requirements: R11-R14; AC9-AC11
- Scope:
  - load the pinned ARMv7 fixture through the dependency graph;
  - apply existing supported relocations first;
  - seal observed GNU RELRO;
  - prove relocated bytes unchanged, RELRO writes fail, and non-RELRO permissions stay unchanged.
- Validation:
  - real fixture CTest + evidence;
  - neighboring real loader/dependency/symbol/relocation tests PASS;
  - exact-head Linux + Android CI PASS.

## T004 — Convergence and final exact-head gate

- Status: QUEUED
- Depends on: T003
- Requirements: R13-R14; AC9-AC12
- Scope:
  - reconcile architecture docs, README, specs/change/evidence, and durable state;
  - preserve lazy-binding/versioning/TLS/IFUNC/global-policy non-goals;
  - run final exact-head CI.
- Validation:
  - Linux CTest PASS;
  - Android x86_64 probe PASS;
  - Android arm64-v8a cross-build PASS;
  - durable state agrees.

## Readiness

PASS. T001/T002 are verified; T003 is the active bounded real-fixture objective.
