# Next Work

Repository state is merged on `bleeding` through PR #31 / runtime commit `2c3be26c05dff81be1f81c9565df5906c521a54c`. `004-elf32-dynamic-placement` is DONE. Exact-head CI #175 PASSed all three jobs before the protected squash merge.

1. Specify and implement the next M4 feature: recursive ELF32 dependency graph/loading.
   - Status: NEXT EXECUTABLE FEATURE SELECTED; spec package not yet created.
   - New feature package: `specs/005-elf32-dependency-loading/` on one substantive branch/PR (planned branch: `m4-elf32-dependency-loading`).
   - Existing prerequisites now available: provider-backed dependency image acquisition from `003`, shared load planning + automatic ET_DYN placement from `004`.
   - Required boundary: keep `elf32_dependency_resolver` acquisition-only; add a new higher layer that owns loaded-object graph/lifetime semantics.
   - Intended first design: provider identity is the opaque object key within one graph-loading operation; repeated/cyclic references reuse an already-known object instead of remapping it; each new ET_DYN image is automatically placed then passed to the unchanged explicit-base `load_elf32`; each newly loaded object is parsed through dynamic → linker metadata → linker strings before its dependencies are traversed.
   - Required resource/safety behavior: explicit graph object/depth/image-byte bounds, checked arithmetic, deterministic traversal, and rollback of mappings created by the graph loader if the aggregate operation fails.
   - Non-goals for this feature: Android/bionic search-path policy, symbol lookup/interposition, relocations/PLT, TLS/RELRO, constructors/destructors, `dlopen`/unload, and execution.
   - Exact next action: create readiness-checked `requirements.md -> design.md -> tasks.md` for `005`, then implement its smallest dependency-ordered slice.
   - DoD for spec round: cycle/dedup identity semantics, root/object ownership, placement/loading order, rollback, resource limits, error categories, and validation are explicit with no unresolved blocker.

2. Capture an Android native crash backtrace/tombstone for the opt-in crash test when an accessible device channel is available.
   - Status: BLOCKED on accessible Android device/environment.
   - Current evidence: crash test armed, emitted `A32CRASH|...|signal=6|...`, and the shell reported `Aborted`.
   - Missing evidence: Android tombstone/native backtrace itself was not included in the captured output.
   - DoD: marker + Android-native crash record are captured from the same explicit `--crash-test` invocation.

3. Collect at least one materially different Android/vendor/kernel sample before making broad fastmem compatibility claims.
   - Status: BLOCKED on another materially different Android/AArch64 environment.
   - Goal: separate implementation confidence from single-device environmental evidence.
   - DoD: environment + reservation/fastmem/fallback observations are recorded as executed evidence.

4. Decide the project's own open-source license before public release.
   - Status: BLOCKED on maintainer choice.
   - Goal: make project redistribution terms explicit while retaining dependency-license auditability.
   - DoD: license file and README/dependency notices are consistent.

Branch hygiene: do not create standalone `state/reconcile-*` branches. Use one branch per substantive feature/fix PR, close state before merge when practical, and delete merged source branches when repository tooling supports it.
