# Next

M2 guest address space is complete for its current scope. Direct Android/AArch64 evidence now covers A32 execution, mapped direct-fastmem data access, and fastmem fault -> callback fallback on the known Android 16 / SDK 36 Termux environment. M3 ELF32 loading is the active milestone.

1. Implement the first bounded M3 ELF32 loader slice.
   - Add an ELF32 parser/validator independent from Dynarmic types.
   - Validate ELF magic/class/data/version, `ET_DYN`/supported type policy, `EM_ARM`, program-header bounds, and overflow cases before mapping anything.
   - Parse `PT_LOAD` segments and compute page-aligned guest mapping ranges/load bias using logical guest VAs.
   - Map segments into `MappedGuestMemory`, copy file bytes, zero-fill BSS (`p_memsz > p_filesz`), and apply final `R`/`RW`/`RX` permissions.
   - Do not implement dynamic symbol resolution or relocations in this first slice.
   - DoD: focused host tests cover one valid synthetic ELF32 image plus malformed headers, out-of-bounds program headers, segment overflow, BSS zero-fill, and final permissions; full existing CTest remains PASS; Android arm64 cross-build remains PASS.

2. Define the M3 loader API boundary before adding linking.
   - Loader must consume a byte image plus `GuestMemory`/mapped-memory services and return guest metadata such as load bias, entry VA, and loaded ranges.
   - Host pointers must not appear in the public loader result.
   - Keep ELF parsing/loading separate from symbol resolution, relocation application, CPU execution, and Android compatibility bridges.

3. Correct the standalone address-space probe environment metadata.
   - Rename compile-time `android.api` to `android.ndk_api` (or equivalent).
   - Report runtime Android SDK/release separately using platform properties, matching `android_runtime_smoke` where practical.
   - Preserve format compatibility where reasonable and document the probe-version change.

4. Add the next focused M1 regressions independently of M3.
   - Add Thumb branch/call coverage.
   - Add Thumb memory/stack coverage.
   - Add targeted exception and invalid-code/memory behavior around the generic CPU seam.
   - Defer broad ISA completeness claims.

5. Collect at least one materially different Android/vendor/kernel sample before broad fastmem compatibility claims.
   - Preserve raw logs under `docs/research/evidence/`.
   - Compare 4 GiB reservation, normal runtime smoke, fastmem direct access, page size, and fault fallback behavior.

6. Add dynamic linking only after the initial ELF32 mapping slice is stable.
   - Introduce ARM relocation handling and symbol resolution as separate work.
   - Preserve loader/linker separation and add malformed relocation/symbol-table tests before real application loading.

7. Exercise fatal crash diagnostics deliberately only after a dedicated safe crash-test mode exists.
   - Verify `A32CRASH|...` survives to file/stderr and Android still emits its native tombstone/backtrace.
   - Do not induce crashes in normal runtime/probe modes merely to test logging.

8. Decide the project's own open-source license before public release.
