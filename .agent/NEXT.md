# Next

`MappedGuestMemory`, Dynarmic fastmem, Linux fallback regressions, Android arm64 packaging, and the normal Android runtime smoke are now proven through direct evidence on the known Android 16 / SDK 36 AArch64 Termux environment.

1. Run the optional Android fastmem fault/fallback mode and persist the raw evidence.
   - Execute `./run.sh --exercise-fastmem-fault` from the existing Termux-private bundle directory.
   - Required evidence: `a32.fastmem_fault.memory_fault=true`, nonzero `a32.fastmem_fault.data_read_callbacks`, `a32.fastmem_fault.status=PASS`, and `runtime_smoke.complete=true`.
   - If the process itself crashes, preserve `A32CRASH|...`, the full file log, and any Android tombstone/backtrace; do not treat a process crash as successful guest-fault recovery.
   - DoD: raw log is stored under `docs/research/evidence/` and Android fastmem fault -> callback fallback is classified from direct device evidence.

2. Reconcile and close M2 after the optional device fallback result.
   - If it passes, promote Android fastmem fault/fallback from NOT RUN to PROVEN on the known environment.
   - Keep callback memory as mandatory correctness fallback and direct low-VA pointer identity outside the generic contract.
   - Keep broad compatibility claims out until additional devices are sampled.

3. Correct the standalone address-space probe's environment metadata before collecting another device sample.
   - Rename compile-time `android.api` to `android.ndk_api` (or equivalent).
   - Report runtime Android SDK/release separately using platform properties, matching the runtime smoke where practical.
   - Preserve probe format compatibility where practical and document the version change.

4. Add the next focused M1 regressions.
   - Add Thumb branch/call coverage.
   - Add Thumb memory/stack coverage.
   - Add targeted exception and invalid-code/memory behavior around the generic CPU seam.
   - Defer broad ISA completeness claims.

5. Begin M3 ELF32 loading as the next vertical runtime slice after M2 device evidence is reconciled.
   - Start with ELF32 validation, `PT_LOAD`, BSS zero-fill, page permissions, and load bias.
   - Load into `MappedGuestMemory` through guest VAs; loader code must not depend on host pointer identity or Dynarmic types.
   - Add malformed/bounds regressions before dynamic linking.

6. Collect at least one materially different Android/vendor/kernel sample before broad fastmem compatibility claims.
   - Preserve raw logs under `docs/research/evidence/`.
   - Compare 4 GiB reservation, runtime-smoke execution, page size, and fault fallback behavior.

7. Exercise fatal crash diagnostics deliberately only after a dedicated safe crash-test mode exists.
   - Verify `A32CRASH|...` survives to file/stderr and Android still emits its native tombstone/backtrace.
   - Do not induce crashes in normal runtime/probe modes merely to test logging.

8. Decide the project's own open-source license before public release.
