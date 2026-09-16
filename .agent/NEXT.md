# Next

PR #9 is merged to `bleeding` as `7357a4dee2c10724630353d6e5a0fd2634115348`, and post-merge GitHub Actions run `35111139535` (#63) is PASS. M3 remains the active milestone.

1. Extend M3 loader metadata to identify `PT_DYNAMIC` without performing linking.
   - Parse program-header metadata needed to locate a loaded PT_DYNAMIC range.
   - Return its guest virtual address/range after load bias in guest-address terms only.
   - Define policy for missing PT_DYNAMIC and reject malformed/multiple conflicting PT_DYNAMIC ranges explicitly.
   - Validate that the reported dynamic range is inside a mapped readable PT_LOAD region.
   - Use the reproducible real ARM32 fixture as the positive integration case; it has observed PT_DYNAMIC at pre-bias guest VA `0x826c` with size `0x60`.
   - Do not interpret DT_NEEDED, DT_REL, symbols, strings, GNU hash, or apply relocations in this slice.
   - DoD: synthetic malformed cases plus the real fixture metadata case PASS; existing 17 tests remain green; Android arm64 cross-build remains PASS.

2. Add actual 16 KiB Android host-page evidence when an appropriate device/runner is available.
   - Run the mapped-memory/runtime diagnostics on an Android AArch64 environment whose runtime page size is 16384.
   - Exercise the real 16 KiB-aligned ARM32 fixture through the loader on that host if a suitable diagnostic entry point exists by then.
   - Keep the current status explicit: 16 KiB ELF metadata is TESTED on a 4 KiB Linux host; actual 16 KiB `MappedGuestMemory` runtime behavior is NOT RUN.

3. Correct the standalone address-space probe environment metadata.
   - Rename compile-time `android.api` to `android.ndk_api` (or equivalent).
   - Report runtime Android SDK/release separately using platform properties, matching `android_runtime_smoke` where practical.

4. Add the next focused M1 regressions independently of M3.
   - Add Thumb branch/call coverage.
   - Add Thumb memory/stack coverage.
   - Add targeted exception and invalid-code/memory behavior around the generic CPU seam.

5. Begin M4 dynamic linking only after M3 mapping/metadata behavior is stable.
   - Add dynamic string/symbol table parsing, DT_NEEDED resolution and ARM relocations as separate tested layers.
   - Preserve loader/linker separation and add malformed metadata/relocation tests before real application loading.

6. Collect at least one materially different Android/vendor/kernel sample before broad fastmem compatibility claims.

7. Exercise fatal crash diagnostics deliberately only after a dedicated safe crash-test mode exists.

8. Decide the project's own open-source license before public release.
