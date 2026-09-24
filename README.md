# liba32android

An experimental, game-agnostic AArch32 compatibility runtime intended to execute 32-bit ARM Android native code inside an AArch64 Android process.

The project is intentionally layered: CPU execution, guest memory, ELF32 mapping, dynamic-linker metadata/semantics, ABI transitions, compatibility libraries and application profiles are separate concerns. Minecraft PE 0.15.x is a future stress target, not the architecture.

## Current phase

The current runtime baseline is C++20/CMake with Dynarmic pinned behind `src/cpu/`. The guest address space, ELF32 mapping/structural parsing, validated linker metadata including separate main-REL and AArch32 PLT-REL descriptors, bounded SONAME/`DT_NEEDED` string consumption, bounded provider-backed dependency image acquisition, deterministic automatic `ET_DYN` guest-VA placement, transactional recursive dependency-graph loading, bounded SysV/GNU dynamic-symbol indexing, exact per-object lookup, deterministic graph-local breadth-first symbol lookup, bounded transactional main-`DT_REL` ARM relocation application, bounded eager PLT `R_ARM_JUMP_SLOT` application, and verified GNU RELRO metadata/sealing primitives are implemented. Real post-relocation RELRO fixture integration is implemented but not yet exact-head validated. Symbol version matching, lazy binding/`DT_PLTGOT`, process-wide/global-group interposition policy, and TLS remain later linker work.

Implemented in the current baseline:

- A32 ARM/Thumb execution through an internal Dynarmic adapter;
- engine-independent `memory::GuestMemory`;
- `LinearGuestMemory` for deterministic callback/correctness tests;
- `MappedGuestMemory` with logical 32-bit guest VAs, a high-base contiguous 4 GiB reservation, mapping/protection lifecycle, Dynarmic fastmem, and callback fallback;
- validated ARM ELF32 `ET_EXEC` / explicit-base `ET_DYN` `PT_LOAD` mapping with file copy, BSS zero-fill, alignment checks, final permissions, conflict checks, and loader-owned rollback;
- validated optional `PT_DYNAMIC` guest-range discovery;
- structural `Elf32_Dyn` parsing from guest memory with raw signed tags/raw values, unknown-tag preservation, and required `DT_NULL` termination;
- validated linker metadata for STRTAB/STRSZ, SYMTAB/SYMENT, main REL/RELSZ/RELENT, AArch32 `DT_JMPREL`/`DT_PLTRELSZ`/`DT_PLTREL=DT_REL`, SONAME, and ordered `DT_NEEDED` offsets, including checked load-bias rebasing and guest-range validation;
- bounded STRTAB string consumption with explicit caller-selected per-string limits, optional SONAME materialization, and ordered/repeated `DT_NEEDED` name preservation;
- provider-backed dependency image acquisition with exact ordered request forwarding, duplicate preservation, distinct provider errors, opaque provider identities, and explicit dependency-count/per-image/total-image resource ceilings;
- a transactional dependency-graph loader that uses provider identity as the per-call object key, preserves ordered/repeated edges, reuses cycles and aliases without remapping, automatically places/loads `ET_DYN` dependencies, enforces graph-wide bounds, retains owned object metadata/images, and rolls back graph-owned mappings on aggregate failure;
- bounded ELF32 symbol indexing and lookup through `DT_HASH` / `DT_GNU_HASH`, including exact byte-name matching, checked logical guest values, explicit unsupported version/TLS/IFUNC boundaries, and graph-local BFS scope derived from dependency edges rather than object-vector order;
- bounded main-`DT_REL` ARM relocation planning/resolution/application for `R_ARM_NONE`, `R_ARM_RELATIVE`, `R_ARM_GLOB_DAT`, and `R_ARM_ABS32`, with Android-compatible `GLOB_DAT` semantics, unresolved-weak-to-zero policy, duplicate-target rejection, plan-before-write validation, and reverse rollback on late write failure;
- separate bounded PLT REL planning/resolution/application for eager `R_ARM_JUMP_SLOT`, reusing the graph-local symbol policy and transactional rollback while writing `S` directly and treating the original slot word only as rollback state;
- a shared immutable ELF32 load plan plus deterministic, bounded, non-mutating low-to-high first-fit `ET_DYN` placement that preserves host-page and `PT_LOAD p_align` constraints while returning an explicit loader-ready `dynamic_base`;
- reproducible Android NDK ARMv7 fixtures: the original loader fixture for loader/dynamic/linker/symbol/main-relocation coverage, plus a freestanding provider/consumer DSO pair with a real `DT_NEEDED` + `R_ARM_JUMP_SLOT fixture_import` path used for graph-backed eager PLT application coverage.

Still outside the implemented baseline:

- Android search-path/namespace/pathname policy and process-wide loaded-object/link-map lifetime across independent graph-loading calls;
- symbol version matching and Android/process-wide global-group interposition beyond the implemented graph-local unversioned lookup;
- lazy PLT binding/`DT_PLTGOT`, combined main+PLT atomic application, and ARM relocation forms beyond the implemented main-REL set plus eager `R_ARM_JUMP_SLOT`;
- RELRO real-fixture closeout and TLS processing;
- Android libc/JNI/graphics/audio compatibility layers;
- end-to-end execution of the real ARM32 ELF fixture on Android;
- general application/game compatibility.

## Validation evidence

Feature `006-elf32-symbol-resolution` is complete. T004 real-fixture validation passed exact-head GitHub Actions run `35837480789` (#206) at `2fdba16a13e34370483701345de2605df06811e6`, and the T005 final feature-head gate passed run `35847914558` (#207) at `ad022c2cc569c3175ad1cef0140f964817f5a820`: Linux A32 smoke passed 41/41 CTest including `elf32_symbol_index` and `elf32_real_symbol_lookup`; Android x86_64 address-space probe and Android arm64-v8a cross-build jobs also passed.

Feature `007-elf32-relocations` is complete. T005 documentation/state/spec convergence and the final feature-head gate PASSed exact-head CI #218 / run `35918899544` at `8efe792cfa58a3f34e02dfe0c8bb01fbc3949766`: Linux passed 45/45 CTest including `elf32_relocation_plan`, `elf32_relocation_apply`, `elf32_real_relocation_plan`, and `elf32_real_relocation_apply`; Android x86_64 address-space probe and Android arm64-v8a cross-build also passed. R1-R15 / AC1-AC15 are reconciled with no recorded semantic gap blocking the bounded feature scope.

Feature `008-elf32-plt-relocation-metadata` is complete. T001 required-job validation passed CI #222 / run `35933694619` at `9a81ed71a027beb166970bcf137bac9a71112f98`, including the explicit pinned-fixture no-PLT oracle. T002 documentation/spec/state convergence and the final exact-head gate PASSed CI #223 / run `35934340806` at `79e9d8c90824baf76d7ff382661422af17e3cb6e`: Linux A32 smoke, Android x86_64 address-space probe, and Android arm64-v8a cross-build all PASSed. The validated metadata layer exposes a separate AArch32 PLT REL descriptor.

Feature `009-elf32-jump-slot-relocations` is complete. T001 read-only PLT planning/reference resolution PASSed CI #226 / run `35938429972` at `fe12b6de747884a18d1214f564559d94937d8974`; T002 transactional eager application PASSed CI #227 / run `35938885569` at `666a15ab2edaebdfa3c0f6817dca30e2e2e7a931`; T003 real provider/consumer integration PASSed CI #228 / run `35939575947` at `815386149732201ce5b64e1b5ad207079491eb80`; and T004 convergence/final exact-head gate PASSed CI #229 / run `35940125841` at `4b255695a9effbaab4028708cd5e7e5a5e23150e`. Linux passed 46/46 CTest including `elf32_real_jump_slot_apply`, and both Android jobs passed. The real consumer declares `DT_NEEDED liba32android_jump_slot_provider.so` and `R_ARM_JUMP_SLOT fixture_import`; graph-backed eager application rewrites that slot to the provider's logical guest symbol value without guest execution.

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

The maintainer's generic agent workflow, runtime capability rules, and governance are centralized in `Millesant/.gpt` and are intentionally not vendored into this repository. `AGENTS.md` is only the project-specific overlay. `.agent/project.toml` declares project identity, accepted current contracts live under `.agent/specs/`, and substantial work lives under `.agent/changes/<change-id>/`. The numbered root `specs/` packages are retained as historical pre-v7 feature records and are not canonical current truth.

Architecture and evidence details remain under `docs/architecture/` and `docs/research/`. The current linker boundaries are documented in `docs/architecture/elf32-linker-metadata.md`, `docs/architecture/elf32-linker-strings.md`, `docs/architecture/elf32-dependency-resolution.md`, `docs/architecture/elf32-dependency-loading.md`, `docs/architecture/elf32-symbol-resolution.md`, and `docs/architecture/elf32-relocation.md`.
