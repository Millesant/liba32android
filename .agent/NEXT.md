# Next

PR #10 is merged to `bleeding` as `d3ff97566c30417eb7bc37f6dd8aed76751ce070`, and post-merge GitHub Actions run `35203647032` (#70) is PASS. M3 remains the active milestone.

1. Parse the loaded `Elf32_Dyn` array structurally without starting dynamic linking.
   - Consume only the already validated `Elf32LoadResult::dynamic_segment` range; keep all addresses in guest-VA terms.
   - Define a small intermediate metadata representation for raw `d_tag`/`d_val` entries without resolving dependencies, symbols, strings, relocations, or GNU hash data.
   - Require complete 8-byte ELF32 dynamic entries inside the file-backed PT_DYNAMIC bytes and a terminating `DT_NULL`; reject truncated or unterminated arrays explicitly before any linker-layer work.
   - Preserve unknown tags as raw metadata rather than silently treating them as supported semantics.
   - Use the reproducible real ARM32 fixture as the positive integration case; its dynamic array contains observed REL/SYMTAB/STRTAB/GNU_HASH-related tags.
   - Do not dereference dynamic pointers or load `DT_NEEDED` dependencies in this slice.
   - DoD: synthetic valid/truncated/unterminated cases plus real-fixture structural parsing PASS; existing 18 tests remain green; Android arm64-v8a cross-build remains PASS; loader/linker separation remains documented.

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
   - Add dynamic string/symbol table validation, DT_NEEDED resolution and ARM relocations as separate tested layers.
   - Preserve loader/linker separation and add malformed metadata/relocation tests before real application loading.

6. Collect at least one materially different Android/vendor/kernel sample before broad fastmem compatibility claims.

7. Exercise fatal crash diagnostics deliberately only after a dedicated safe crash-test mode exists.

8. Decide the project's own open-source license before public release.
