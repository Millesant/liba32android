# Next Work

Repository state is merged on `bleeding` through PR #29 / commit `004bb8a42f3be2638e7a700c6c6a83bf73117a83`. The runtime code baseline remains PR #23 / `709e9ce74e58ee12d925b64c8466b9afa58cc4a6`. The CI #134 runtime-smoke artifact has now been executed successfully in Termux for both the ordinary smoke and the explicit SIGABRT crash-test path.

1. Merge the converged Android 16 KiB ELF-alignment fix, then keep the AArch64 16 KiB runtime test as the remaining architecture-specific gap.
   - Status: CONVERGED on exact head `dad047a71636974173da6b14c388df09ea58deb9`.
   - GitHub Actions: #148 PASS — Linux A32 smoke, Android x86_64 address-space probe, Android arm64-v8a cross-build.
   - Fedora emulator: PASS — Android 15 / SDK 35 / x86_64 / PAGE_SIZE=16384.
   - ELF gate: all observed x86_64 probe `PT_LOAD` entries use `p_align=0x4000`.
   - Runtime evidence: 4 GiB reservation/commit PASS; sampled exact low-VA mappings + EEXIST collisions PASS; RW->RX PASS; generated x86-64 return-42 PASS; harness final PASS.
   - Exact next action: merge PR #30 after the persistence-only exact-head CI gate passes.
   - Remaining boundary: AArch64 `liba32android.so` / Dynarmic runtime execution on a 16 KiB AArch64 Android target remains NOT RUN.

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
