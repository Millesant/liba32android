# liba32android

An experimental, game-agnostic AArch32 compatibility runtime intended to execute 32-bit ARM Android native code inside an AArch64 Android process.

The project is intentionally layered: CPU execution, guest memory, ELF32 mapping, dynamic-linker metadata/semantics, ABI transitions, compatibility libraries and application profiles are separate concerns. Minecraft PE 0.15.x is a future stress target, not the architecture.

## Current phase

The current runtime baseline is C++20/CMake with Dynarmic pinned behind `src/cpu/`. The guest address space, ELF32 mapping/structural parsing, validated linker metadata, bounded SONAME/`DT_NEEDED` string consumption, bounded provider-backed dependency image acquisition, deterministic automatic `ET_DYN` guest-VA placement, and transactional recursive dependency-graph loading are implemented. Symbol semantics and relocation application remain later linker work.

Implemented in the current baseline:

- A32 ARM/Thumb execution through an internal Dynarmic adapter;
- engine-independent `memory::GuestMemory`;
- `LinearGuestMemory` for deterministic callback/correctness tests;
- `MappedGuestMemory` with logical 32-bit guest VAs, a high-base contiguous 4 GiB reservation, mapping/protection lifecycle, Dynarmic fastmem, and callback fallback;
- validated ARM ELF32 `ET_EXEC` / explicit-base `ET_DYN` `PT_LOAD` mapping with file copy, BSS zero-fill, alignment checks, final permissions, conflict checks, and loader-owned rollback;
- validated optional `PT_DYNAMIC` guest-range discovery;
- structural `Elf32_Dyn` parsing from guest memory with raw signed tags/raw values, unknown-tag preservation, and required `DT_NULL` termination;
- validated linker metadata for STRTAB/STRSZ, SYMTAB/SYMENT, REL/RELSZ/RELENT, SONAME, and ordered `DT_NEEDED` offsets, including checked load-bias rebasing and guest-range validation;
- bounded STRTAB string consumption with explicit caller-selected per-string limits, optional SONAME materialization, and ordered/repeated `DT_NEEDED` name preservation;
- provider-backed dependency image acquisition with exact ordered request forwarding, duplicate preservation, distinct provider errors, opaque provider identities, and explicit dependency-count/per-image/total-image resource ceilings;
- a transactional dependency-graph loader that uses provider identity as the per-call object key, preserves ordered/repeated edges, reuses cycles and aliases without remapping, automatically places/loads `ET_DYN` dependencies, enforces graph-wide bounds, retains owned object metadata/images, and rolls back graph-owned mappings on aggregate failure;
- a shared immutable ELF32 load plan plus deterministic, bounded, non-mutating low-to-high first-fit `ET_DYN` placement that preserves host-page and `PT_LOAD p_align` constraints while returning an explicit loader-ready `dynamic_base`;
- a reproducible Android NDK ARMv7 ELF fixture used for loader, dynamic-array, linker-metadata, linker-string, zero-dependency resolver, automatic-placement, and dependency-graph integration coverage.

Still outside the implemented baseline:

- Android search-path/namespace/pathname policy and process-wide loaded-object/link-map lifetime across independent graph-loading calls;
- dynamic symbol-table consumption, hash lookup, and symbol lookup/interposition;
- ARM relocations;
- RELRO and TLS processing;
- Android libc/JNI/graphics/audio compatibility layers;
- end-to-end execution of the real ARM32 ELF fixture on Android;
- general application/game compatibility.

## Validation evidence

PR #32 final head `1ac47ef59f3570989d6fc07cd187c129cbe76588` passed GitHub Actions run `35712896172` (#199): Linux A32 smoke passed 39/39 CTest including `elf32_dependency_loading` and `elf32_real_dependency_loading`; Android x86_64 address-space probe and Android arm64-v8a cross-build jobs also passed. PR #32 was squash-merged to `bleeding` as `17c2aa78535adbd2084c9396f525750e10c0eff8`; the squash commit preserved the exact validated source tree.

Post-merge GitHub Actions run `35204765081` (#73) on `bleeding` commit `ac008b2d2a3158ffa4cb285e88cfabacea2ca4a2` is **PASS**:

- Linux configure/build: PASS;
- reproducible pinned-NDK ARM32 fixture generation and byte-identical regeneration: PASS;
- CTest: 20/20 PASS, including the real ELF32 fixture and structural dynamic-array integration cases;
- exact `liba32android.so` output-name check: PASS;
- Android `arm64-v8a` runtime + diagnostics configure/build/link: PASS;
- Android runtime/probe/runtime-smoke artifact production: PASS.

The repository also contains recorded Android/AArch64 device evidence for the mapped-memory/fastmem path on one known Android 16 / SDK 36 Termux environment, plus a PASS of the standalone x86_64 Android 15 / 16 KiB address-space/JIT probe. Those observations do not establish universal device compatibility. AArch64 runtime execution on a 16 KiB Android host and loading/executing the current real ARM32 fixture on Android remain **NOT RUN**.

## Build

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DLIBA32ANDROID_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

GitHub Actions also cross-builds the shared runtime and Android diagnostics for `arm64-v8a` with NDK `27.3.13750724`.

## Project workflow metadata

The maintainer's generic agent workflow, runtime capability rules, and governance are centralized in `Millesant/.gpt` and are intentionally not vendored into this repository. `AGENTS.md` is only the project-specific overlay. Durable project continuation state lives under `.agent/`, while feature-scale project specifications use the requirements -> design -> tasks packages under `specs/`; `specs/000-current-baseline/` converts the completed runtime work through PR #11 into that structure.

Architecture and evidence details remain under `docs/architecture/` and `docs/research/`. The current linker boundaries are documented in `docs/architecture/elf32-linker-metadata.md`, `docs/architecture/elf32-linker-strings.md`, `docs/architecture/elf32-dependency-resolution.md`, and `docs/architecture/elf32-dependency-loading.md`.
