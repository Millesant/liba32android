# Tasks — ELF32 ARM eager JUMP_SLOT relocations

Status: READY

Base revision: `bbad7db39e430ba87a373841e3ede954696c85c5`
Control revision: `9240ab19507b86491398a5c9fdf0deb58e2fdc91`

## T001 — PLT REL planning and reference resolution

- Status: ACTIVE
- Requirements: R1-R7, R13-R14; AC1-AC5, AC8
- Scope:
  - add `kRArmJumpSlot = 22`;
  - factor existing relocation planning/resolution internally without changing main REL behavior;
  - add separate PLT plan + resolution APIs consuming `plt_rel_table`;
  - PLT planner accepts only JUMP_SLOT;
  - add synthetic decode/count/read/place/alignment/type/duplicate/reference/weak/strong coverage.
- Validation:
  - `elf32_relocation_plan` synthetic suite covers main + PLT behavior;
  - existing main relocation apply tests remain green;
  - exact-head Linux and Android CI jobs PASS.
- Stop condition: read-only PLT planning/resolution is validated; do not write PLT targets yet.

## T002 — Transactional eager JUMP_SLOT application

- Status: QUEUED
- Depends on: T001
- Requirements: R8-R10, R13-R14; AC5-AC8
- Scope:
  - add `apply_elf32_plt_rel_relocations`;
  - final word is `S`, original word ignored semantically;
  - reuse plan-before-write + reverse rollback engine;
  - add non-zero-original, weak-zero, late-write rollback, rollback-failure coverage.
- Validation:
  - focused synthetic PLT application cases;
  - all existing main relocation tests PASS;
  - exact-head Linux + Android CI PASS.

## T003 — Reproducible real JUMP_SLOT fixture and graph-backed application

- Status: QUEUED
- Depends on: T002
- Requirements: R11-R12, R14; AC9-AC11
- Scope:
  - add freestanding ARMv7 provider/consumer fixture sources + pinned-NDK builder;
  - CI builds pair twice and byte-compares;
  - inspect exact `DT_NEEDED` and `R_ARM_JUMP_SLOT fixture_import`;
  - add real dependency-graph + PLT application CTest;
  - verify resolved provider guest value, mapping permissions, and non-target bytes.
- Validation:
  - fixture build/readelf evidence;
  - new real JUMP_SLOT test PASS;
  - neighboring real dependency/symbol/relocation tests PASS;
  - exact-head Linux + Android CI PASS.

## T004 — Convergence and final exact-head gate

- Status: QUEUED
- Depends on: T003
- Requirements: R13-R14; AC8-AC12
- Scope:
  - reconcile architecture docs, README, specs, change state/evidence, `.agent/STATE.md`, and `.agent/NEXT.md`;
  - preserve explicit lazy-binding/versioning/TLS/IFUNC/RELRO/global-policy non-goals;
  - run final exact-head CI.
- Validation:
  - Linux CTest PASS;
  - Android x86_64 probe PASS;
  - Android arm64-v8a cross-build PASS;
  - durable change/spec/docs/state agree.

## Readiness

PASS. T001 is the active bounded objective.
