# Tasks — ELF32 Dynamic Guest Placement

## T001 — Add deterministic free guest-range search
- Requirements: R2, R4, R5, R7 / AC2, AC4
- Depends on: none
- Scope:
  - add `src/memory/guest_va_allocator.{h,cpp}`;
  - expose a non-mutating low-to-high first-fit search over `const MappedGuestMemory&`;
  - require explicit search window, length, alignment and alignment offset;
  - use checked arithmetic and `MappedGuestMemory::page_size()`;
  - add focused host tests and CMake/CTest wiring.
- Validation:
  - first candidate PASS;
  - conflict skip PASS;
  - offset/alignment PASS;
  - 16 KiB-equivalent alignment PASS;
  - overflow/invalid/no-space failures PASS;
  - mapped state/permissions unchanged after every search.
- Status: DONE — exact-head CI #156 PASS on `81f56630d9f6b499d360320ea5e6d4b640e3fb88`; Linux A32 smoke, Android x86_64 address-space probe, and Android arm64-v8a cross-build all PASS.

## T002 — Refactor loader validation into a reusable immutable load-layout plan
- Requirements: R1, R3, R6, R7 / AC3, AC5, AC8
- Depends on: T001
- Scope:
  - extract the existing pre-mutation ELF validation/planning logic without changing accepted/rejected behavior;
  - expose the minimum mapped page, maximum mapped end/span, ELF type and combined load-bias alignment needed by placement;
  - make `load_elf32` consume the shared plan;
  - do not alter explicit-base loader semantics.
- Validation:
  - existing `elf32_loader_test` cases remain PASS;
  - new plan tests cover ET_DYN/ET_EXEC type, extent and 0x4000 alignment.
- Status: DONE — PR-head CI #162 PASS on `c804c4d9c6eb8ffd5784c396aa14a3f7a158fb17`; Linux A32 smoke, Android x86_64 address-space probe, and Android arm64-v8a cross-build all PASS.

## T003 — Add automatic ET_DYN placement API
- Requirements: R2-R8 / AC1-AC6
- Depends on: T001, T002
- Scope:
  - add `src/elf/elf32_dynamic_placement.{h,cpp}`;
  - accept an image, `const MappedGuestMemory&` and explicit search options;
  - reject non-ET_DYN;
  - translate the validated load layout into the generic range-search constraints;
  - return one explicit `dynamic_base` only; do not map.
- Validation:
  - first-fit synthetic placement PASS;
  - occupied candidate skip PASS;
  - bounded exhaustion -> NoSpace;
  - malformed/ET_EXEC rejection;
  - returned base passed unchanged to `load_elf32` -> successful load.
- Status: DONE — PR-head CI #166 PASS on `5fe92029e4b7107f9aca97b1505e4f8dee5c8f3d`; Linux A32 smoke, Android x86_64 address-space probe, and Android arm64-v8a cross-build all PASS.

## T004 — Integrate the real ARM32 fixture
- Requirements: R1, R3, R7, R8 / AC7, AC8
- Depends on: T003
- Scope:
  - add fixture-backed automatic-placement integration;
  - preserve existing fixture generation and explicit-base tests;
  - require the selected base/load bias to satisfy the fixture's `p_align=0x4000`.
- Validation:
  - pinned fixture auto-placement + load PASS;
  - existing real-fixture loader/dynamic/linker/resolver integration remains PASS.
- Status: DONE — PR-head CI #169 PASS on `053c6435b902eb0b0f6412b9d63a44e32274d060`; real-fixture auto-placement evidence, full Linux CTest, Android x86_64 probe build, and Android arm64-v8a cross-build all PASS.

## T005 — Converge feature and durable state
- Requirements: all / AC1-AC8
- Depends on: T004
- Scope:
  - update architecture docs and README current phase;
  - reconcile `.agent/STATE.md` / `.agent/NEXT.md`;
  - exact-head CI gate before merge.
- Validation:
  - Linux CTest PASS;
  - Android x86_64 probe cross-build PASS;
  - Android arm64-v8a runtime/diagnostics cross-build PASS;
  - requirements/design/code/tests/state convergence has no unrecorded material gap.
- Status: ACTIVE — convergence/docs/state updated; final exact-head CI NOT RUN.

## Readiness Check

- [x] Every acceptance criterion has an implementation/validation path.
- [x] Every task traces to a requirement/design need.
- [x] No unresolved question blocks safe implementation.
- [x] Design preserves D-0003/D-0004 and loader/dependency-provider layering.
- [x] Existing explicit `ET_DYN dynamic_base` behavior remains compatible.
- [x] Placement is non-mutating and retains loader conflict detection.
- [x] Checked 32-bit guest-address/resource boundaries are explicit.
- [x] Task dependencies are acyclic and executable.
- [x] T001 is small enough for a restart-safe bounded implementation round.
