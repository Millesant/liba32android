# Tasks — ELF32 Dependency Graph Loading

## T001 — Add a transactional root-object graph loader
- Requirements: R1, R2, R5, R6, R8-R12 / AC1, AC2, AC11-AC14
- Depends on: none
- Scope:
  - add `src/elf/elf32_dependency_loader.{h,cpp}`;
  - define owned root/options/result/node/edge/error contracts;
  - validate graph options and root image/identity limits before mutation;
  - support dependency-free root `ET_EXEC` fixed loading and root `ET_DYN` automatic placement;
  - run the existing dynamic → metadata → strings pipeline for the root;
  - represent missing `PT_DYNAMIC` as empty linker/dependency state;
  - record successful mappings and roll back the root on post-load parse/string failure;
  - add focused host tests and CMake/CTest wiring.
- Validation:
  - dependency-free ET_EXEC root PASS;
  - dependency-free ET_DYN placement -> exact-base load PASS;
  - no-PT_DYNAMIC root PASS with empty downstream state;
  - invalid root/options/resource failures occur before guest mutation;
  - post-load pipeline failure removes root mappings;
  - unrelated preexisting mapping remains unchanged.
- Status: DONE — exact-head CI #182 PASS on `9a1d5bf4e97656724fdb7d649197af0d8cdd1ed8`; Linux A32 smoke, Android x86_64 address-space probe, and Android arm64-v8a cross-build all PASS. Focused `elf32_dependency_loading_root` coverage is included in the Linux test step.\n\n## T002 — Load direct dependencies with identity-based graph reuse
- Requirements: R3-R8, R10-R12 / AC3, AC5, AC6, AC8-AC10, AC12, AC14
- Depends on: T001
- Scope:
  - call the existing `resolve_elf32_dependencies` for the root's complete direct dependency set;
  - preserve ordered/repeated dependency edges;
  - use provider identity as the object key;
  - map each first-seen dependency once via automatic ET_DYN placement + unchanged explicit-base loader;
  - reject ET_EXEC dependencies;
  - retain owned unique object images/metadata;
  - compare later equal-identity image bytes against the first image and reject mismatches;
  - enforce graph-wide object/occurrence/image/string budgets across direct dependencies.
- Validation:
  - ordered direct dependencies PASS;
  - repeated name -> repeated edges, one mapped object when identity matches;
  - alias names -> one object when provider identity matches;
  - equal identity + different bytes -> explicit failure + rollback;
  - ET_EXEC dependency rejection + rollback;
  - provider/resource failures expose no partial graph.
- Status: TODO

## T003 — Add bounded recursion, cycles, and aggregate rollback
- Requirements: R3, R4, R8-R11 / AC4, AC7, AC10, AC11
- Depends on: T002
- Scope:
  - add Discovered/Loading/Loaded internal object state;
  - recursively process each new object's dynamic/metadata/string/dependency pipeline;
  - preserve deterministic depth-first edge traversal after each object's direct acquisition;
  - terminate cycles by reusing Loading/Loaded identities;
  - enforce max depth and graph-wide occurrence/image/object budgets through recursion;
  - roll back all graph-owned successful mappings in reverse load order on any later failure;
  - report rollback failure distinctly if an owned unmap unexpectedly fails.
- Validation:
  - transitive A -> B -> C PASS;
  - cycle A -> B -> A PASS with two mapped objects;
  - shared dependency A -> B,C and B -> C maps C once;
  - max-depth failure is deterministic;
  - later transitive acquisition/parse/placement/load failure removes all earlier graph-owned mappings;
  - unrelated preexisting guest mappings remain mapped with original permissions.
- Status: TODO

## T004 — Integrate the pinned real ARM32 fixture
- Requirements: R5, R6, R11-R13 / AC15
- Depends on: T003
- Scope:
  - add fixture-backed graph-loader integration without removing existing explicit-base tests;
  - use a provider that fails if invoked because the current fixture has zero `DT_NEEDED`;
  - require automatic placement/load through the graph API;
  - retain fixture metadata/string assertions already established by neighboring integration tests;
  - require the fixture's `0x4000` load-bias alignment constraint when dynamic.
- Validation:
  - real fixture graph load PASS;
  - zero dependency edges PASS;
  - zero provider calls PASS;
  - required alignment remains `0x4000`;
  - existing real-fixture loader/dynamic/metadata/string/resolver tests remain PASS.
- Status: TODO

## T005 — Converge architecture, durable state, and exact-head CI
- Requirements: all / AC1-AC16
- Depends on: T004
- Scope:
  - update README/current-phase and dependency-loading architecture docs;
  - reconcile `.agent/STATE.md` / `.agent/NEXT.md`;
  - compare requirements/design/code/tests for material gaps;
  - run exact-head CI before protected merge.
- Validation:
  - Linux CTest PASS;
  - Android x86_64 address-space probe cross-build PASS;
  - Android arm64-v8a runtime/diagnostics cross-build PASS;
  - requirements/design/code/tests/docs/state convergence has no unrecorded material gap.
- Status: TODO

## Readiness Check

- [x] Every acceptance criterion has an implementation/validation path.
- [x] Every task traces to a requirement/design need.
- [x] No unresolved question blocks safe implementation.
- [x] Design preserves D-0003/D-0004 and existing ELF/resolver/placement layering.
- [x] Existing explicit-base `load_elf32` behavior remains compatible.
- [x] Provider pathname/namespace policy remains outside the generic graph loader.
- [x] Provider identity, duplicate, alias, cycle, and identity/image-mismatch semantics are explicit.
- [x] Root/dependency ELF-type rules are explicit.
- [x] Aggregate resource bounds and checked accounting are explicit.
- [x] Rollback ownership excludes preexisting guest mappings.
- [x] Symbol/relocation/runtime semantics are explicitly deferred.
- [x] Task dependencies are acyclic and each active slice is restart-safe.
