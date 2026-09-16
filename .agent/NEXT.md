# Next

Real Android/AArch64 evidence now exists and D-0004 selects a high-base contiguous 4 GiB reservation as the preferred first fastmem acceleration path, with callbacks retained as the mandatory correctness fallback.

1. Implement the first mapped guest-address-space backend.
   - Reserve a contiguous 4 GiB host range without requiring a low host VA.
   - Keep unmapped guest pages inaccessible and track guest mapping/permission metadata explicitly.
   - Support page-aligned map, protect and unmap lifecycle operations needed by ELF `PT_LOAD` later.
   - Expose the reservation base only through an internal capability suitable for Dynarmic fastmem; do not expose host pointers as guest/runtime API values.
   - Preserve callback reads/writes as the correctness fallback for fastmem faults and unsupported accesses.
   - DoD: Linux tests cover mapping, permissions, unmapping, bounds and callback fallback; Android arm64 cross-build passes.

2. Wire optional Dynarmic fastmem to the mapped backend.
   - Set `UserConfig::fastmem_pointer` only when the memory backend owns a valid contiguous 4 GiB reservation.
   - Keep `recompile_on_fastmem_failure` enabled so protected/unmapped pages can fall back to callbacks.
   - Add targeted data load/store tests that exercise the same CPU seam with and without fastmem capability.
   - DoD: callback-only and fastmem-capable memory backends produce identical guest-visible results in tests.

3. Add an Android/AArch64 runtime smoke executable and artifact.
   - Execute the existing A32 return-42 test through Dynarmic's AArch64 backend on device.
   - Add a guest load/store case to exercise the mapped backend/fastmem path when implemented.
   - Reuse file diagnostics so the user can run it from Termux without adb or Wi-Fi and return one log file.
   - DoD: real Android device evidence shows A32 guest execution returns 42 through `liba32android`.

4. Correct probe environment metadata before the next device sample.
   - Rename compile-time `android.api` to `android.ndk_api` (or equivalent).
   - Report runtime Android SDK/release separately using platform properties.
   - Preserve probe format compatibility where practical and document the version change.

5. Collect additional Android/vendor/kernel samples.
   - Repeat the address-space probe on at least one materially different Android environment before broad compatibility claims.
   - Preserve raw logs under `docs/research/evidence/` and classify observations separately from inference.

6. Exercise crash diagnostics deliberately only after a safe dedicated crash-test mode exists.
   - Verify `A32CRASH|...` survives to the file/stderr and Android still emits its native tombstone/backtrace.
   - Do not induce crashes in normal runtime/probe modes merely to test logging.

7. After the guest-address-space backend and CPU fastmem seam are stable, design M3 ELF32 loading.
   - Start with validation, `PT_LOAD`, BSS, permissions and load bias before dynamic linking.
   - Keep ELF/linker responsibilities independent from the CPU engine.

8. Decide the project's own open-source license before public release.
