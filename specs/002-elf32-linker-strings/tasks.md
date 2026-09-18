# Tasks — ELF32 Linker Strings

## T001 — Add bounded linker-string reader and API
- Requirements: R1-R7, R10; AC3-AC9
- Depends on: merged `001-elf32-linker-metadata`
- Scope: add `src/elf/elf32_linker_strings.{h,cpp}`; define explicit options/result/error types plus independently callable `read_elf32_string_table_entry`; implement checked STRTAB offset/address handling, bounded chunked NUL scanning, explicit max-length enforcement, byte-preserving materialization, and no-mutation behavior.
- Validation: focused single-entry synthetic cases for empty/non-UTF-8 strings, out-of-range offsets, overflow, read failure, unterminated strings, exact limit, over-limit strings, and unchanged guest bytes.
- Status: DONE — CI #97 PASS

## T002 — Materialize SONAME and ordered NEEDED names
- Requirements: R8-R10; AC1-AC2, AC10
- Depends on: T001
- Scope: build the aggregate API on `read_elf32_string_table_entry`; require STRTAB when SONAME/NEEDED is requested; consume optional SONAME and ordered/repeated NEEDED offsets from `Elf32LinkerMetadata`; preserve order/duplicates; make aggregate success all-or-nothing.
- Validation: synthetic SONAME + multiple NEEDED cases, repeated offsets/names, missing-STRTAB rejection, no-strings/no-STRTAB success, and a later-NEEDED failure that does not yield successful partial output.
- Status: IMPLEMENTED — CI NOT RUN

## T003 — Integrate the reproducible real ARM32 fixture
- Requirements: R11; AC11
- Depends on: T001, T002
- Scope: load the real fixture, parse dynamic metadata, build linker metadata, consume strings with an explicit ceiling, assert SONAME `liba32android_loader_fixture.so`, and assert zero NEEDED names.
- Validation: fixture-backed CTest integration.
- Status: TODO

## T004 — Converge architecture and durable state
- Requirements: R1, R11; AC12
- Depends on: T001-T003
- Scope: document the linker-string boundary and explicit resource-limit contract; update README and `.agent/STATE.md` / `.agent/NEXT.md`; keep dependency loading and pathname policy explicitly deferred.
- Validation: consistency pass across requirements/design/tasks/code/tests/docs/state.
- Status: TODO

## T005 — CI gate
- Requirements: R11; AC12
- Depends on: T001-T004
- Scope: run GitHub Actions on the exact feature head.
- Validation:
  - synthetic linker-string tests PASS;
  - real fixture linker-string integration PASS;
  - existing host suite PASS;
  - reproducible ARM32 fixture generation PASS;
  - exact `liba32android.so` naming PASS;
  - Android `arm64-v8a` runtime/diagnostics cross-build PASS.
- Status: TODO

## Readiness Check

- [x] Every acceptance criterion has an implementation/validation path.
- [x] Every task traces to a requirement/design need.
- [x] No unresolved question blocks safe implementation.
- [x] Design preserves D-0003/D-0004 and the loader/dynamic/metadata layering.
- [x] Compatibility impact is explicit: no existing ELF API contract is changed.
- [x] Security/resource boundaries are explicit and caller-bounded.
- [x] Task dependencies are acyclic and executable.
- [x] The first implementation slice (T001) is small enough for a restart-safe bounded round.
