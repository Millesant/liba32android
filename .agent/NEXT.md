# Next

The first M3 ELF32 `PT_LOAD` mapping slice is merged. PR #9 adds a reproducible real ARMv7/Android ET_DYN fixture, real PT_LOAD integration evidence, and PT_LOAD-aligned ET_DYN load-bias validation. M3 remains the active milestone.

1. Integrate PR #9 after the final documented-head CI is green.
   - Keep the PR bounded to reproducible fixture generation, loader-only real ELF validation, evidence capture, and p_align-preserving ET_DYN load bias.
   - Do not fold dynamic linking or relocation application into the merge.
   - DoD: PR #9 merged to `bleeding`, post-merge CI PASS, durable state reconciled.

2. Extend M3 loader metadata to identify `PT_DYNAMIC` without performing linking.
   - Parse program-header metadata needed to locate a loaded PT_DYNAMIC range.
   - Return its guest virtual address/range after load bias in guest-address terms only.
   - Define policy for missing PT_DYNAMIC and reject malformed/multiple conflicting PT_DYNAMIC ranges explicitly.
   - Validate that the reported dynamic range is inside a mapped readable PT_LOAD region.
   - Use the reproducible real ARM32 fixture as the positive integration case; it has observed PT_DYNAMIC at pre-bias guest VA `0x826c` with size `0x60`.
   - Do not interpret DT_NEEDED, DT_REL, symbols, strings, GNU hash, or apply relocations in this slice.
   - DoD: synthetic malformed cases plus the real fixture metadata case PASS; existing 17 tests remain green; Android arm64 cross-build remains PASS.

3. Add actual 16 KiB Android host-page evidence when an appropriate device/runner is available.
   - Run the mapped-memory/runtime diagnostics on an Android AArch64 environment whose runtime page size is 16384.
   - Exercise the real 16 KiB-aligned ARM32 fixture through the loader on that host if a suitable diagnostic entry point exists by then.
   - Keep the current status explicit: 16 KiB ELF metadata is TESTED on a 4 KiB Linux host; actual 16 KiB `MappedGuestMemory` runtime behavior is NOT RUN.

4. Correct the standalone address-space probe environment metadata.
   - Rename compile-time `android.api` to `android.ndk_api` (or equivalent).
   - Report runtime Android SDK/release separately using platform properties, matching `android_runtime_smoke` where practical.

5. Add the next focused M1 regressions independently of M3.
   - Add Thumb branch/call coverage.
   - Add Thumb memory/stack coverage.
   - Add targeted exception and invalid-code/memory behavior around the generic CPU seam.

6. Begin M4 dynamic linking only after M3 mapping/metadata behavior is stable.
   - Add dynamic string/symbol table parsing, DT_NEEDED resolution and ARM relocations as separate tested layers.
   - Preserve loader/linker separation and add malformed metadata/relocation tests before real application loading.

7. Collect at least one materially different Android/vendor/kernel sample before broad fastmem compatibility claims.

8. Exercise fatal crash diagnostics deliberately only after a dedicated safe crash-test mode exists.

9. Decide the project's own open-source license before public release.
