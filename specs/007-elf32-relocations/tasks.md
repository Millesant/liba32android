# Tasks — ELF32 ARM REL Relocation Application

## T001 — Add bounded read-only REL decoding/planning

- Requirements: R1-R6, R11, R13 / AC1-AC3, AC10
- Depends on: feature 006 DONE
- Scope:
  - add `elf32_relocation.{h,cpp}`;
  - explicit 8-byte little-endian `Elf32_Rel` decode;
  - bounded `max_relocations`;
  - checked `load_bias + r_offset` place calculation;
  - 4-byte alignment and target-word read for supported write-producing forms;
  - preserve original word/addend;
  - represent `R_ARM_NONE` without target access;
  - classify only NONE/ABS32/GLOB_DAT/RELATIVE as plan-supported;
  - reject duplicate writable targets;
  - no guest mutation.
- Validation:
  - synthetic decode/count/read/overflow/alignment/unsupported/duplicate/no-mutation cases;
  - pinned real fixture plan reports exactly two GLOB_DAT entries with symbol indexes 2/3, linked offsets 0x82cc/0x82d0, and zero original words.
- Documentation:
  - public header must state guest-address, limits, supported-type, addend, and read-only plan contracts.
- Status: DONE — exact-head CI #210 / run `35850236188` PASSed at `2b4e9185bac43fe9bb46ddf8c7da9b73e0146837`: Linux passed 43/43 CTest including `elf32_relocation_plan` and `elf32_real_relocation_plan`; Android x86_64 and arm64-v8a jobs also PASSed.

## T002 — Add bounded relocation-reference symbol decoding/resolution

- Requirements: R7-R10, R13 / AC5-AC9
- Depends on: T001
- Scope:
  - decode relocation-referenced dynsym entry by index under the already built symbol extent;
  - materialize exact reference name;
  - enforce binding/visibility/type/section/version boundaries;
  - reject protected-reference semantics in this feature;
  - graph-local BFS resolution starting at relocating object;
  - strong not-found failure;
  - unresolved weak -> S=0 for supported absolute forms;
  - retain nested symbol/index/string errors.
- Validation:
  - bounds/name/binding/visibility/version/unsupported-form failures;
  - graph-local success;
  - weak unresolved behavior.
- Status: DONE — exact-head CI #212 / run `35889244367` PASSed at `650d7b262540360ba2395a802ba7d7766566d544`: Linux passed 43/43 CTest including the expanded `elf32_relocation_plan` and `elf32_real_relocation_plan`; Android x86_64 and arm64-v8a jobs also PASSed.

## T003 — Apply NONE/RELATIVE/GLOB_DAT/ABS32 transactionally

- Requirements: R5-R12 / AC3-AC11
- Depends on: T002
- Scope:
  - compute all final words before mutation;
  - RELATIVE requires symbol index zero and writes B+A modulo 2^32;
  - GLOB_DAT writes S and ignores in-place A;
  - ABS32 writes S+A modulo 2^32;
  - writes in table order;
  - reverse rollback on late write failure;
  - explicit rollback-failure result;
  - no permission changes.
- Validation:
  - success formulas including nonzero-addend GLOB_DAT;
  - pre-write failure leaves memory unchanged;
  - later write failure restores earlier words;
  - rollback-failure backend when practical.
- Status: DONE — exact-head CI #214 / run `35890660951` PASSed at `41a93348c29fb884befba5ba8bad51ecf0d49665`: Linux passed 44/44 CTest including `elf32_relocation_apply`; Android x86_64 and arm64-v8a jobs also PASSed.

## T004 — Integrate pinned real ARM32 GLOB_DAT fixture

- Requirements: R14-R15 / AC12-AC14
- Depends on: T003
- Scope:
  - graph-load pinned fixture;
  - resolve fixture_bss and fixture_data expected values through feature 006;
  - apply object-0 main REL table;
  - require places load_bias+0x82cc / +0x82d0 to equal the expected guest values;
  - preserve provider-call count, mappings and permissions;
  - preserve initialized data/BSS contents.
- Validation:
  - new real-fixture relocation CTest PASS;
  - all neighboring real loader/dynamic/metadata/string/dependency/symbol tests PASS.
- Status: DONE — exact-head CI #216 / run `35891830738` PASSed at `5d74af22c16a7bc99eee7038dfb9f137b22807c2`: Linux passed 45/45 CTest including `elf32_real_relocation_apply`; Android x86_64 and arm64-v8a jobs also PASSed.

## T005 — Converge docs/state/spec and final exact-head gate

- Requirements: all / AC1-AC15
- Depends on: T004
- Scope:
  - add/update `docs/architecture/elf32-relocation.md`;
  - reconcile README, linker/symbol boundary docs, spec, `.agent/STATE.md`, and `.agent/NEXT.md`;
  - compare requirements/design/code/tests for semantic gaps;
  - run final exact-head CI.
- Validation:
  - Linux CTest PASS;
  - Android x86_64 address-space probe PASS;
  - Android arm64-v8a cross-build PASS;
  - docs/contracts match implemented mutation/rollback semantics.
- Status: ACTIVE — implementation/test/spec review found no blocking R1-R15 / AC1-AC14 semantic gap; documentation/state/spec convergence is prepared and AC15 final exact-head CI remains pending.

## Readiness check

- [x] Feature ID 007 is unused.
- [x] REL metadata dependency is already implemented and validated.
- [x] Graph-local symbol lookup dependency is DONE.
- [x] The first supported relocation set is explicit.
- [x] PLT/JMPREL/TLS/IFUNC/versioning/global-group/text-relocation policy is out of scope.
- [x] Android-specific GLOB_DAT addend semantics are explicit and evidence-backed.
- [x] Weak unresolved behavior is assigned to relocation policy.
- [x] Mutation is plan-before-write with rollback.
- [x] Duplicate REL targets have deterministic first-feature behavior.
- [x] Resource ceilings and checked guest-place arithmetic are explicit.
- [x] The pinned fixture provides a real GLOB_DAT oracle.
- [x] T001 is a bounded restart-safe implementation slice.
