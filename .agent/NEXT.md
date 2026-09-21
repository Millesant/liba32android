# Next Work

Repository state is merged on `bleeding` through PR #31 / runtime commit `2c3be26c05dff81be1f81c9565df5906c521a54c`. `004-elf32-dynamic-placement` is DONE. Exact-head CI #175 PASSed all three jobs before the protected squash merge.

1. Capture an Android native crash backtrace/tombstone for the opt-in crash test when an accessible device channel is available.
   - Current evidence: crash test armed, emitted `A32CRASH|...|signal=6|...`, and the shell reported `Aborted`.
   - Missing evidence: Android tombstone/native backtrace itself was not included in the captured output.
   - DoD: marker + Android-native crash record are captured from the same explicit `--crash-test` invocation.

2. Collect at least one materially different Android/vendor/kernel sample before making broad fastmem compatibility claims.
   - Goal: separate implementation confidence from single-device environmental evidence.
   - Depends on: access to another Android/AArch64 environment.
   - DoD: environment + reservation/fastmem/fallback observations are recorded as executed evidence.

3. Decide the project's own open-source license before public release.
   - Goal: make project redistribution terms explicit while retaining dependency-license auditability.
   - Depends on: maintainer choice.
   - DoD: license file and README/dependency notices are consistent.

Branch hygiene: do not create standalone `state/reconcile-*` branches. Use one branch per substantive feature/fix PR, close state before merge when practical, and delete merged source branches when repository tooling supports it.
