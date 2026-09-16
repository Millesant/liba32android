# Next

`MappedGuestMemory`, the internal Dynarmic fastmem seam, Linux fastmem/fallback regressions, and an Android arm64 runtime-smoke bundle are implemented. GitHub CI proves Linux execution behavior and Android arm64 compile/link/package behavior; actual A32 execution through Dynarmic's AArch64 backend is the next decisive device test.

1. Run `android_runtime_smoke` on the known Android/AArch64 Termux environment and persist the raw evidence.
   - Download/extract the `android-runtime-smoke-<sha>` artifact as a bundle.
   - Copy the bundle into a Termux-private executable directory; do not execute from shared `/sdcard` storage.
   - Run `./run.sh` first.
   - Preserve the complete `liba32android-runtime-smoke.log` emitted through `diagnostics.log_file=`.
   - Required normal-smoke evidence: `a32.return42.r0=42`, `a32.return42.status=PASS`, `a32.memory.fastmem_direct=true`, `a32.memory.status=PASS`, and `runtime_smoke.complete=true`.
   - DoD: raw log is stored under `docs/research/evidence/` with runtime SDK/release/kernel metadata and Android A32 execution can be labeled PASS/FAIL from direct evidence.

2. After the normal Android runtime smoke passes, run the optional fastmem fault/fallback mode.
   - Execute `./run.sh --exercise-fastmem-fault`.
   - This intentionally accesses an unmapped guest page; Dynarmic should recover the host fastmem fault and route the guest access to callbacks rather than crashing the process.
   - Required evidence: `a32.fastmem_fault.memory_fault=true`, nonzero `a32.fastmem_fault.data_read_callbacks`, `a32.fastmem_fault.status=PASS`, and `runtime_smoke.complete=true`.
   - If the process itself crashes, preserve `A32CRASH|...`, the file log, and any Android tombstone/backtrace; do not silently treat a process crash as a successful guest fault.

3. Reconcile M2 after real Android runtime-smoke evidence.
   - If both normal execution and fault fallback pass, promote Android/AArch64 fastmem execution from NOT RUN to PROVEN on that environment.
   - Keep callbacks as the mandatory correctness fallback and keep direct low-VA pointer identity outside the generic contract.
   - Do not make broad compatibility claims from one device; retain the environment-specific evidence boundary.
   - DoD: `STATE.md`, architecture docs, and evidence logs agree on what is PROVEN vs TESTED vs NOT RUN.

4. Correct the standalone address-space probe's environment metadata before collecting another device sample.
   - Rename compile-time `android.api` to `android.ndk_api` (or equivalent).
   - Report runtime Android SDK/release separately using platform properties, matching the runtime smoke where practical.
   - Preserve probe format compatibility where practical and document the version change.

5. Add the next focused M1 regressions independently of device availability.
   - Add Thumb branch/call coverage.
   - Add Thumb memory/stack coverage.
   - Add targeted exception and invalid-code/memory behavior around the generic CPU seam.
   - Defer broad ISA completeness claims.

6. Once the Android runtime-smoke evidence is satisfactory, begin M3 ELF32 loading as the next vertical runtime slice.
   - Start with ELF32 validation, `PT_LOAD`, BSS zero-fill, page permissions, and load bias.
   - Load into `MappedGuestMemory` through guest VAs; loader code must not depend on host pointer identity or Dynarmic types.
   - Add malformed/bounds regressions before dynamic linking.

7. Collect at least one materially different Android/vendor/kernel sample before broad fastmem compatibility claims.
   - Preserve raw logs under `docs/research/evidence/`.
   - Compare 4 GiB reservation, runtime-smoke execution, page size, and fault fallback behavior.

8. Exercise fatal crash diagnostics deliberately only after a dedicated safe crash-test mode exists.
   - Verify `A32CRASH|...` survives to file/stderr and Android still emits its native tombstone/backtrace.
   - Do not induce crashes in normal runtime/probe modes merely to test logging.

9. Decide the project's own open-source license before public release.
