# Android arm64 device evidence — 2026-09-16

Source: `android_address_space_probe` executed from Termux on a real AArch64 Android device. Raw output is preserved verbatim in `android-termux-arm64-2026-09-16.log`.

## Evidence classification

### Observed

- The probe executed as an AArch64 Android ELF in Termux and completed normally.
- File logging to an explicit Termux-accessible Downloads path succeeded.
- Direct default selection of `/sdcard/Download/liba32android/address-space-probe.log` was reported unavailable in this process context; the explicit Termux storage path worked.
- Host page size was 4096 bytes.
- `/proc/sys/vm/mmap_min_addr` was unreadable from the process (`EACCES`).
- `/proc/self/maps` contained zero mappings below 4 GiB at probe time.
- A contiguous 4 GiB `PROT_NONE` reservation succeeded at host base `0x7888058000`, which is above 4 GiB.
- A page within that 4 GiB reservation could be changed to read/write, written/read successfully, restored, and the reservation could be unmapped.
- `MAP_FIXED_NOREPLACE` produced exact mappings at all six sampled guest-like addresses: `0x10000`, `0x100000`, `0x1000000`, `0x10000000`, `0x40000000`, and `0x80000000`.
- A second mapping request at each occupied sampled address failed with `EEXIST`, demonstrating collision protection for those samples.
- An anonymous page could transition from RW to RX.
- With generated-code execution enabled, AArch64 code `mov w0,#42; ret` executed successfully and returned 42.
- No crash occurred, so `A32CRASH|...` fatal-signal behavior and tombstone coexistence remain NOT RUN.

### Important metadata caveat

The raw probe line `android.api=26` comes from the compile-time `__ANDROID_API__` macro and therefore identifies the NDK/platform target used to build the probe, not the device's runtime Android SDK level. Do not use that line as evidence that the device itself runs API 26. A follow-up probe revision should label the field as an NDK API target and separately report runtime Android SDK/release metadata.

### Inferred

- A high-base contiguous 4 GiB host reservation is a viable fastmem candidate in this observed Termux process. This is directly compatible with Dynarmic's `fastmem_pointer` model, which accepts an arbitrary host base for the guest 4 GiB address space.
- Direct low-VA mapping is technically available at the sampled addresses in this process, but it is not required for Dynarmic fastmem and should not become a generic runtime invariant.
- RW-to-RX JIT memory transitions and execution are viable in this observed process context.

### Not yet proven

- Cross-device / cross-vendor compatibility of the 4 GiB reservation.
- The same low-VA availability inside an arbitrary app process with different loaded libraries and Android runtime state.
- Actual Dynarmic A32 guest execution through the AArch64 backend on this device.
- Fastmem page-fault fallback behavior through `liba32android` on Android.
- Runtime mapped-region lifecycle, permissions, guard pages, cache invalidation, or ELF loading.

## Architectural consequence

This evidence is sufficient to select high-base 4 GiB reservation as the first Android fastmem acceleration path while retaining callback memory as the mandatory correctness fallback. It is not sufficient to require direct low-VA guest-pointer identity.
