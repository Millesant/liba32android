# Next

The generic CPU/memory seam is validated, the Android address-space probe is cross-built, and the probe now persists copy-pasteable diagnostics/crash markers when Android permits file creation. Device behavior is still NOT RUN.

1. Execute `android_address_space_probe` on representative Android arm64 environments and persist the raw evidence.
   - Start with the default non-generated-code mode; then run `--execute-generated-code` explicitly.
   - The probe first tries `/sdcard/Download/liba32android/address-space-probe.log`; use the emitted `diagnostics.log_file=` path if it falls back elsewhere.
   - Send/store the complete log, including `diagnostics.*`, Android build/API, kernel release, page size, `mmap_min_addr`, low-4-GiB mappings, 4 GiB reservation result, `MAP_FIXED_NOREPLACE` results, RW->RX result and generated-code return value.
   - If the process crashes, preserve any `A32CRASH|...` marker and Android tombstone/native backtrace in addition to the file log.
   - Prefer more than one Android/kernel generation before making a compatibility claim.
   - Store outputs under `docs/research/evidence/` with probe commit and device/kernel metadata.
   - DoD: observed device results are separated from inference and are sufficient to compare callback/page-table/fastmem/direct-low-VA strategies, and real logging/crash-marker behavior is recorded as PASS/FAIL rather than assumed.

2. After device evidence exists, choose the first production guest-address-space backend.
   - Keep callbacks as mandatory correctness fallback.
   - Compare page-table acceleration against fastmem using observed constraints.
   - Do not choose direct low-VA pointer identity unless evidence demonstrates a safe compatibility envelope and fallback.
   - DoD: accepted decision records mapping lifecycle, permissions, fallback behavior, cache invalidation interaction and Android constraints.

3. While device execution is unavailable, continue independent M1 regression coverage.
   - Add Thumb branch/call and memory/stack coverage.
   - Add exception and invalid-memory behavior around the generic CPU seam.
   - Defer broad ISA completeness claims; targeted regressions prove runtime integration behavior.

4. Add an Android/AArch64 A32 execution test when a suitable runner/device path is available.
   - Current CI proves Android `arm64-v8a` compile/link only.
   - DoD: execute the A32 return-42 smoke through Dynarmic's AArch64 backend on Android and record evidence.

5. When the public runtime/app integration layer is introduced, add host-configured diagnostics.
   - Do not hard-code `/sdcard/Download` inside the generic embedded runtime.
   - Accept an app-provided writable directory or callback/sink; let the Android host export/share logs through platform-appropriate storage APIs.
   - Reuse stable `A32ERR|...` and fatal `A32CRASH|...` shapes where applicable.
   - DoD: runtime errors and fatal markers survive into a host-controlled diagnostic artifact without requiring logcat as the only source.

6. After the guest-address-space contract is selected and stable enough, design M3 ELF32 loading.
   - Start with validation, PT_LOAD, BSS, permissions and load bias before dynamic linking.
   - Keep ELF/linker responsibilities independent from the CPU engine.

7. Decide the project's own open-source license before public release.
