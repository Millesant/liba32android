# Tasks — ELF32 Symbol Resolution

## T001 — Add hash metadata and bounded dynamic-symbol indexing

- Requirements: R1-R6, R14-R16 / AC1-AC3, AC13
- Depends on: none
- Scope:
  - extend linker metadata with singleton/rebased `DT_HASH` and `DT_GNU_HASH` descriptors;
  - validate fixed hash headers at metadata stage without parsing variable arrays;
  - add `elf32_symbol_lookup.{h,cpp}` index contracts and explicit options;
  - parse/validate bounded SysV hash and derive count from `nchain`;
  - parse/validate bounded ELF32 GNU hash and derive count when GNU-only;
  - when both formats exist, use SysV count as full extent while validating GNU indexes inside it;
  - validate the complete bounded dynamic-symbol table range;
  - keep all reads through `GuestMemory`;
  - document resource/mutation/guest-address contracts in the new public header.
- Validation:
  - focused linker-metadata hash-tag cases;
  - valid/malformed/overflow/limit SysV index cases;
  - valid/malformed/unterminated/limit GNU cases;
  - both-hash bounds;
  - symbol-table range/read validation;
  - existing metadata/string/dependency regressions remain green.
- Status: DONE — exact-head CI #203 / run `35758444356` PASSed at `45cd3e5a322263f53dc02277a9d0e801849515db`: Linux passed 40/40 CTest including `elf32_symbol_index`; Android x86_64 and arm64-v8a jobs also PASSed.

## T002 — Add exact-name per-object symbol lookup

- Requirements: R7-R11, R14-R16 / AC4-AC10, AC12-AC13
- Depends on: T001
- Scope:
  - implement SysV and GNU name hashing/chain traversal;
  - decode `Elf32_Sym` entries byte-explicitly;
  - reuse bounded STRTAB entry reads for candidate names;
  - enforce binding/visibility/definition eligibility;
  - implement ABS vs checked rebased guest-value semantics;
  - return symbol index/raw symbol/guest value;
  - explicitly reject matching TLS/common/XINDEX/IFUNC-or-unknown semantics;
  - detect unsupported version metadata before unsafe name-only resolution.
- Validation:
  - exact-name success on both hash styles;
  - collision traversal;
  - local/undefined/hidden/internal skip;
  - global/weak/protected success;
  - ABS and rebased value cases;
  - string/read/value overflow failures;
  - unsupported matching symbol forms;
  - no guest mutation.
- Status: DONE — exact-head CI #204 / run `35759553586` PASSed at `5f21c8ed48f458f7f3d909fff39523d9ebf9b7e0`: Linux passed 40/40 CTest including the expanded `elf32_symbol_index`; Android x86_64 and arm64-v8a jobs also PASSed.

## T003 — Add deterministic graph-local breadth-first lookup

- Requirements: R12-R16 / AC7, AC11-AC13
- Depends on: T002
- Scope:
  - derive BFS scope from dependency edges, not object-vector order;
  - preserve per-object edge order;
  - visit cycles/shared objects once;
  - skip objects with no symbol table;
  - fail on malformed/unsupported searchable objects encountered before a definition;
  - return first eligible definition;
  - preserve first-definition weak behavior;
  - enforce explicit scope-object ceiling.
- Validation:
  - deliberately shuffled object-vector graph;
  - BFS order;
  - cycles/shared targets/repeated edges;
  - first-definition and weak-before-global behavior;
  - earlier malformed object failure;
  - scope limit;
  - read-only behavior.
- Status: DONE — exact-head CI #205 / run `35760283793` PASSed at `f3997d037f7f5a29b1666dd9a6f5a566b249a2cc`: Linux passed 40/40 CTest including the expanded `elf32_symbol_index`; Android x86_64 and arm64-v8a jobs also PASSed.

## T004 — Integrate the pinned real ARM32 GNU-hash fixture

- Requirements: R2-R10, R17 / AC14-AC15
- Depends on: T003
- Scope:
  - load the pinned fixture through the graph API;
  - build its GNU symbol index;
  - resolve `fixture_add`, `fixture_data`, and `fixture_bss`;
  - require object index 0 for all three;
  - require `fixture_data == 0x12345678` through resolved guest memory;
  - require `fixture_bss == 0`;
  - require `fixture_add` value lies in an executable mapped segment;
  - keep existing fixture tests unchanged.
- Validation:
  - new fixture-backed symbol-resolution CTest PASS;
  - neighboring real fixture loader/dynamic/metadata/string/dependency tests PASS.
- Status: ACTIVE — pinned real ARM32 GNU-hash graph lookup, resolved data/BSS/function checks, executable-segment validation, and read-only snapshot coverage are implemented for the next exact-head gate.

## T005 — Converge docs/state/spec and run exact-head CI

- Requirements: all / AC1-AC16
- Depends on: T004
- Scope:
  - add/update `docs/architecture/elf32-symbol-resolution.md`;
  - reconcile README, metadata/dependency boundary docs, spec, `.agent/STATE.md`, and `.agent/NEXT.md`;
  - compare requirements/design/code/tests for unrecorded semantic gaps;
  - run exact-head CI.
- Validation:
  - Linux CTest PASS;
  - Android x86_64 address-space probe cross-build PASS;
  - Android arm64-v8a runtime/diagnostics cross-build PASS;
  - docs/contracts match implemented lookup/hash/scope semantics.
- Status: BLOCKED — depends on T004

## Readiness Check

- [x] Every acceptance criterion has an implementation/validation path.
- [x] Every task traces to requirements/design.
- [x] Hash-derived symbol count works without section headers.
- [x] SysV and GNU hash formats have explicit bounded parsing rules.
- [x] Symbol binding/visibility/value semantics are explicit.
- [x] Unsupported TLS/common/XINDEX/IFUNC/versioning behavior is explicit rather than silently approximated.
- [x] Feature 005 object-vector order is not repurposed as symbol scope.
- [x] Graph-local scope uses deterministic BFS over dependency edges.
- [x] Weak-vs-global runtime behavior is explicit.
- [x] Resource ceilings and checked arithmetic are explicit.
- [x] No relocation writes/pathname/namespace/global-group policy leaks into this feature.
- [x] Real fixture validation targets known exported GNU-hash symbols.
- [x] T001 is a bounded restart-safe implementation slice.
