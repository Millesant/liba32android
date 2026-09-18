# Tasks — ELF32 Dependency Resolution

## T001 — Add provider boundary and ordered dependency acquisition
- Requirements: R1-R7; AC1-AC5
- Depends on: merged `002-elf32-linker-strings`
- Scope: add `src/elf/elf32_dependency_resolver.{h,cpp}`; define provider/result/source/options types; implement empty-set success, dependency-count precheck, empty-name rejection, exact byte forwarding, one provider call per ordered occurrence, duplicate preservation, and host-owned successful results.
- Validation: deterministic fake-provider tests for ordered success, repeated names/calls, zero dependencies/zero calls, empty-name rejection before provider invocation, non-UTF-8 bytes, slash-containing names, and owned result bytes.
- Status: TODO

## T002 — Enforce provider failures and resource budgets
- Requirements: R5-R9; AC6-AC10
- Depends on: T001
- Scope: translate provider not-found vs provider-failure distinctly; reject empty identity/image; enforce per-image and total-byte ceilings with checked 64-bit arithmetic; pass the bounded request ceiling to the provider; defensively reject oversized provider results; keep aggregate failure all-or-nothing.
- Validation: synthetic provider cases for NotFound, Failed, empty identity, empty image, zero/limited per-image budget, limited total budget, provider contract violation, checked total accounting, and a later-occurrence failure with no successful partial aggregate.
- Status: TODO

## T003 — Integrate the real ARM32 fixture zero-dependency path
- Requirements: R10-R11; AC11-AC12
- Depends on: T001, T002
- Scope: extend/add fixture-backed integration through loader → dynamic → linker metadata → linker strings → dependency resolver; use a provider that fails if called; require successful empty dependency output and zero provider calls.
- Validation: fixture-backed CTest integration while preserving the fixture's known zero-`DT_NEEDED` property.
- Status: TODO

## T004 — Converge architecture and durable state
- Requirements: R1-R2, R10-R11; AC11-AC13
- Depends on: T001-T003
- Scope: document the provider-owned lookup boundary, explicit acquisition resource limits, duplicate-preservation policy, and the deliberate stop before guest placement/link-map semantics; update README and `.agent/STATE.md` / `.agent/NEXT.md`.
- Validation: consistency pass across requirements/design/tasks/code/tests/docs/state.
- Status: TODO

## T005 — CI gate
- Requirements: R11; AC13
- Depends on: T001-T004
- Scope: run GitHub Actions on the exact feature head.
- Validation:
  - synthetic dependency-resolution tests PASS;
  - real fixture zero-dependency integration PASS;
  - existing loader/dynamic/metadata/string tests PASS;
  - reproducible ARM32 fixture generation PASS;
  - exact `liba32android.so` naming PASS;
  - Android `arm64-v8a` runtime/diagnostics cross-build PASS.
- Status: TODO

## Readiness Check

- [x] Every acceptance criterion has an implementation/validation path.
- [x] Every task traces to a requirement/design need.
- [x] No unresolved question blocks safe implementation.
- [x] Design preserves D-0003/D-0004 and the loader/dynamic/metadata/string layering.
- [x] Compatibility impact is explicit: existing ELF APIs and the explicit `ET_DYN dynamic_base` contract remain unchanged.
- [x] Security/resource boundaries are explicit: dependency count, per-image bytes, and total image bytes are caller-bounded.
- [x] Filesystem/search-path/namespace policy is assigned explicitly to the injected provider rather than left ambiguous.
- [x] Duplicate policy is explicit: preserve occurrences in this layer; defer dedup/link-map identity semantics.
- [x] Guest placement, recursion, symbol lookup, and relocations are explicitly out of scope.
- [x] Task dependencies are acyclic and executable.
- [x] The first implementation slice (T001) is small enough for a restart-safe bounded round.
