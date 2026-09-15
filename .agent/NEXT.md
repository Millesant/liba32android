# Next

M0 bootstrap CI validation is complete for the currently implemented scope.

1. Define the M1/M2 execution and guest-address-space seam before expanding instruction tests.
   - Replace the 4 KiB callback-memory scaffold with a generic guest memory/address-space interface.
   - Keep Dynarmic-specific types inside `src/cpu/`.
   - Add focused guest tests for register state, branches/calls, stack use and memory load/store.
   - DoD: all new tests have explicit PASS/FAIL evidence in CI; no unexecuted behavior is marked working.

2. Research and test Android address-space strategies independently.
   - Measure low-VA reservation feasibility, `MAP_FIXED_NOREPLACE`, executable mappings, ASLR collisions and guard regions across relevant Android versions/devices.
   - Compare callback memory, page-table mode and Dynarmic fastmem.
   - Keep direct low-address mapping as HYPOTHESIS until device evidence exists.

3. Add an Android/AArch64 execution test when a suitable runner/device path is available.
   - Current CI proves Android `arm64-v8a` compile/link only.
   - DoD: execute the A32 return-42 smoke through the AArch64 backend on Android hardware/emulation and record evidence.

4. After the CPU/address-space seam is stable, design M3 ELF32 loading.
   - Start with validation/PT_LOAD/BSS/perms/load bias before dynamic linking.
   - Keep ELF/linker responsibilities independent from the CPU engine.

5. Decide the project's own open-source license before public release.
