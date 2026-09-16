# Next

The first M3 ELF32 `PT_LOAD` mapping slice is merged to `bleeding` as `8059c07b989f845b5b48e306e39cdca6c66d502b` and post-merge GitHub Actions run `35086133758` (#52) is PASS. M3 remains the active milestone.

1. Add a reproducible real ARM32 ELF fixture for loader-only compatibility evidence.
   - Prefer generating a tiny freestanding ARMv7/Android `ET_DYN` fixture reproducibly from source rather than committing an opaque application binary.
   - Feed the fixture bytes through the host ELF32 loader and validate load bias, mapped ranges, file bytes, BSS and final permissions.
   - Record the fixture's observed `PT_LOAD` layout, alignment and page-boundary behavior as evidence.
   - Explicitly include 16 KiB host-page compatibility in the evidence plan: current synthetic loader tests execute on a 4 KiB Ubuntu host and do not prove shared-page behavior on 16 KiB Android hosts.
   - If the real fixture demonstrates page-overlapping `PT_LOAD` ranges, implement an explicit shared-page mapping plan without silently granting RWX; otherwise retain the current rejection.
   - DoD: fixture generation is reproducible, loader-only integration test PASS, synthetic malformed tests remain PASS, and the observed page-layout constraints are documented.

2. Extend M3 metadata parsing needed by the future linker without performing linking yet.
   - Identify loaded `PT_DYNAMIC` and relevant read-only metadata by guest VA/range.
   - Keep parser output in guest-address terms only; host pointers must remain absent from the loader/linker boundary.
   - Do not resolve DT_NEEDED, symbols or relocations in this step.

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
