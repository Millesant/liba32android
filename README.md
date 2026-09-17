# liba32android

An experimental, game-agnostic AArch32 compatibility runtime intended to execute 32-bit ARM Android native code inside an AArch64 Android process.

The project is intentionally layered: CPU execution, guest memory, ELF32 mapping, dynamic-linker metadata/semantics, ABI transitions, compatibility libraries and application profiles are separate concerns. Minecraft PE 0.15.x is a future stress target, not the architecture.

## Current phase

The current runtime baseline is C++20/CMake with Dynarmic pinned behind `src/cpu/`. The guest address space and ELF32 mapping/structural-metadata slices are implemented; dynamic-linker semantics are the next feature-scale boundary.

Implemented in the current baseline:

- A32 ARM/Thumb execution through an internal Dynarmic adapter;
- engine-independent `memory::GuestMemory`;
- `LinearGuestMemory` for deterministic callback/correctness tests;
- `MappedGuestMemory` with logical 32-bit guest VAs, a high-base contiguous 4 GiB reservation, mapping/protection lifecycle, Dynarmic fastmem, and callback fallback;
- validated ARM ELF32 `ET_EXEC` / explicit-base `ET_DYN` `PT_LOAD` mapping with file copy, BSS zero-fill, alignment checks, final permissions, conflict checks, and loader-owned rollback;
- validated optional `PT_DYNAMIC` guest-range discovery;
- structural `Elf32_Dyn` parsing from guest memory with raw signed tags/raw values, unknown-tag preservation, and required `DT_NULL` termination;
- a reproducible Android NDK ARMv7 ELF fixture used for loader and dynamic-array integration coverage.

Still outside the implemented baseline:

- `DT_NEEDED` dependency loading;
- dynamic string/symbol table consumption and symbol lookup/interposition;
- ARM relocations;
- RELRO and TLS processing;
- automatic `ET_DYN` guest-VA allocation;
- Android libc/JNI/graphics/audio compatibility layers;
- end-to-end execution of the real ARM32 ELF fixture on Android;
- general application/game compatibility.

## Validation evidence

Post-merge GitHub Actions run `35204765081` (#73) on `bleeding` commit `ac008b2d2a3158ffa4cb285e88cfabacea2ca4a2` is **PASS**:

- Linux configure/build: PASS;
- reproducible pinned-NDK ARM32 fixture generation and byte-identical regeneration: PASS;
- CTest: 20/20 PASS, including the real ELF32 fixture and structural dynamic-array integration cases;
- exact `liba32android.so` output-name check: PASS;
- Android `arm64-v8a` runtime + diagnostics configure/build/link: PASS;
- Android runtime/probe/runtime-smoke artifact production: PASS.

The repository also contains recorded Android/AArch64 device evidence for the mapped-memory/fastmem path on one known Android 16 / SDK 36 Termux environment. That does not establish universal device compatibility. Actual 16 KiB Android host-page behavior and loading/executing the current real ARM32 fixture on a real Android device remain **NOT RUN**.

## Build

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DLIBA32ANDROID_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

GitHub Actions also cross-builds the shared runtime and Android diagnostics for `arm64-v8a` with NDK `27.3.13750724`.

## Project workflow

Repository-local agent rules live in `AGENTS.md`. Durable continuation state lives under `.agent/`. Feature-scale work uses the requirements -> design -> tasks packages under `specs/`; `specs/000-current-baseline/` converts the completed runtime work through PR #11 into that structure.

Architecture and evidence details remain under `docs/architecture/` and `docs/research/`.
