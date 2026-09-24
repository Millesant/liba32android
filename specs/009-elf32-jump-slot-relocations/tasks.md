# Tasks — ELF32 ARM eager JUMP_SLOT relocations

Status: DONE — T001-T004 complete; final exact-head CI #229 PASSed

Base revision: `bbad7db39e430ba87a373841e3ede954696c85c5`
Control revision: `9240ab19507b86491398a5c9fdf0deb58e2fdc91`

## T001 — PLT REL planning and reference resolution

- Status: VERIFIED — exact-head CI #226 / run `35938429972` PASSed at `fe12b6de747884a18d1214f564559d94937d8974` across Linux A32 smoke, Android x86_64, and Android arm64-v8a
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

- Status: VERIFIED — exact-head CI #227 / run `35938885569` PASSed at `666a15ab2edaebdfa3c0f6817dca30e2e2e7a931` across Linux A32 smoke, Android x86_64, and Android arm64-v8a
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

- Status: VERIFIED — exact-head CI #228 / run `35939575947` PASSed at `815386149732201ce5b64e1b5ad207079491eb80`; pinned-NDK double-build/readelf evidence and graph-backed real application all PASSed
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

- Status: DONE — final exact-head CI #229 / run `35940125841` PASSed at `4b255695a9effbaab4028708cd5e7e5a5e23150e` across Linux A32 smoke, Android x86_64, and Android arm64-v8a
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

PASS. T001-T004 are complete and R1-R14 / AC1-AC12 are reconciled. Feature 009 is converged; the next M4 slice must be selected separately.
