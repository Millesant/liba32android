# Next

M2 guest address space is complete for its current scope. The first M3 ELF32 `PT_LOAD` mapping slice is implemented on PR #8 and validated in GitHub Actions with Linux 16/16 PASS plus Android arm64 cross-build PASS.

1. Integrate PR #8 after the final documented-head CI is green.
   - Keep the first slice bounded to ELF32 validation + `PT_LOAD` mapping.
   - Do not fold relocations/symbol resolution into the merge.
   - DoD: PR merged to `bleeding`, post-merge CI PASS, state reconciled.

2. Add a reproducible real ARM32 ELF fixture for loader-only compatibility evidence.
   - Prefer generating a tiny freestanding ARMv7/Android `ET_DYN` fixture reproducibly from source rather than committing an opaque application binary.
   - Feed the fixture bytes through the host ELF32 loader and validate load bias, mapped ranges, file bytes, BSS and final permissions.
   - Record the real fixture's `PT_LOAD` layout/alignment as evidence.
   - If the fixture demonstrates page-overlapping `PT_LOAD` ranges, implement an explicit shared-page plan without silently granting RWX; otherwise retain the current rejection.
   - DoD: fixture generation is reproducible, loader-only integration test PASS, synthetic malformed tests remain PASS.

3. Extend M3 metadata parsing needed by the future linker without performing linking yet.
   - Identify loaded `PT_DYNAMIC` and relevant read-only metadata by guest VA/range.
   - Keep parser output in guest-address terms only.
   - Do not resolve DT_NEEDED, symbols or relocations in this step.

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
