# Next

The initial M1/M2 CPU-to-memory seam is implemented and validated in GitHub Actions.

1. Research and test Android guest-address-space strategies independently.
   - Measure low-VA reservation feasibility, `MAP_FIXED_NOREPLACE`, executable mappings, ASLR collisions and guard regions across relevant Android versions/devices.
   - Compare callback memory, page-table mode and Dynarmic fastmem.
   - Keep direct low-address mapping as HYPOTHESIS until device evidence exists.
   - DoD: research notes distinguish observed device evidence from inference and recommendation.

2. Add an Android/AArch64 execution test when a suitable runner/device path is available.
   - Current CI proves Android `arm64-v8a` compile/link only.
   - DoD: execute the A32 return-42 smoke through the AArch64 backend on Android hardware/emulation and record evidence.

3. Expand M1 instruction coverage only with focused behavior tests.
   - Add Thumb branch/call and memory/stack coverage before relying on those paths in higher layers.
   - Add exception/invalid-memory behavior tests around the generic CPU seam.
   - Defer broad ISA completeness claims; Dynarmic supplies the decoder/JIT and targeted regressions prove runtime integration behavior.

4. After the guest-address-space contract is stable enough, design M3 ELF32 loading.
   - Start with validation, PT_LOAD, BSS, permissions and load bias before dynamic linking.
   - Keep ELF/linker responsibilities independent from the CPU engine.

5. Decide the project's own open-source license before public release.
