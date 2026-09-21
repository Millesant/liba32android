# Next Work

Repository state is merged on `bleeding` through PR #30 / commit `428a76ca7d8335b0198b7a2e26c736bc8dbe198f`. The runtime code baseline remains PR #23 / `709e9ce74e58ee12d925b64c8466b9afa58cc4a6`. The CI #134 runtime-smoke artifact has now been executed successfully in Termux for both the ordinary smoke and the explicit SIGABRT crash-test path.

1. Implement T002 of `specs/004-elf32-dynamic-placement`: reusable immutable ELF load-layout planning.
   - Status: READY; T001 predecessor DONE with exact-head CI #156 PASS at `81f56630d9f6b499d360320ea5e6d4b640e3fb88`.
   - Goal: extract the existing pre-mutation ELF32 validation/planning logic into one reusable immutable plan consumed by `load_elf32` and future placement code.
   - Required plan facts: ELF type, entry, page-aligned minimum load page, maximum load end/span, combined required load-bias alignment, validated PT_LOAD planning data, and optional validated PT_DYNAMIC metadata.
   - Invariants: no accepted/rejected loader behavior change, no mapping before validation, explicit `dynamic_base` semantics unchanged, no duplicate ELF parser.
   - Validation: all existing loader tests PASS plus focused direct plan tests for ET_DYN/ET_EXEC type, mapped extent, and `0x4000` load-bias alignment.
   - DoD: focused plan tests + full Linux CTest + both Android cross-build jobs PASS before T003 automatic placement begins.

2. Capture an Android native crash backtrace/tombstone for the opt-in crash test when an accessible device channel is available.
   - Current evidence: crash test armed, emitted `A32CRASH|...|signal=6|...`, and the shell reported `Aborted`.
   - Missing evidence: Android tombstone/native backtrace itself was not included in the captured output.
   - DoD: marker + Android-native crash record are captured from the same explicit `--crash-test` invocation.

3. Collect at least one materially different Android/vendor/kernel sample before making broad fastmem compatibility claims.
   - Goal: separate implementation confidence from single-device environmental evidence.
   - Depends on: access to another Android/AArch64 environment.
   - DoD: environment + reservation/fastmem/fallback observations are recorded as executed evidence.

4. Decide the project's own open-source license before public release.
   - Goal: make project redistribution terms explicit while retaining dependency-license auditability.
   - Depends on: maintainer choice.
   - DoD: license file and README/dependency notices are consistent.
