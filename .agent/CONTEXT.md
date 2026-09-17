# Project Context

## Mission

Build a reusable, game-agnostic AArch32 compatibility runtime for AArch64 Android. The intended stack is ARM32 Android ELF -> ELF32 loader -> dynamic-linker metadata/semantics -> guest runtime/ABI/compatibility layers -> A32 execution engine -> AArch64 Android host.

Minecraft PE 0.15.x is a future stress target, not the architecture.

## Non-goals

Do not hard-code one game into the generic runtime. Application-specific work belongs under `profiles/` and must not leak into CPU, memory, ELF, ABI, compatibility-library, or platform contracts.

Do not equate guest pointer values with host pointer identity, and do not treat a successful cross-build or one device sample as proof of broad Android compatibility.

## Architecture boundaries

Keep CPU execution, guest address space, ELF32 mapping, structural dynamic metadata, dynamic linking, AAPCS32/AAPCS64 bridging, compatibility libraries, pthread/TLS, signals, JNI, graphics/audio, instrumentation and application profiles separate.

Current dependency direction is intentionally one-way:

```text
ELF image -> ELF32 loader -> GuestMemory
                          -> structural Elf32_Dyn metadata -> future linker
GuestMemory -> CPU adapter -> Dynarmic
```

ELF/ABI/runtime APIs operate on logical 32-bit guest VAs and must not expose host pointers as guest pointers.

## Current phase

The M2 guest-address-space scope is implemented. M3 ELF32 mapping plus structural dynamic-array metadata is implemented and green through PR #11. The next feature-scale boundary is M4-style linker metadata/semantics; dependency loading, symbol resolution and relocations are not implemented.

## Current stack

- Language/build: C++20 + CMake/Ninja.
- CPU engine: Dynarmic behind `src/cpu/`, pinned to `azahar-emu/dynarmic` commit `e77b1ba0b7da7cbe93021b01a663acfe7c4dd516`.
- Generic memory seam: `memory::GuestMemory`.
- Correctness memory: `LinearGuestMemory`.
- Mapped memory: `MappedGuestMemory`, logical 32-bit guest VAs, contiguous high-host-VA 4 GiB reservation, page map/protect/unmap lifecycle.
- CPU acceleration: internal mapped-memory `fastmem_base()` capability plus Dynarmic fastmem; callbacks remain the mandatory correctness fallback.
- ELF: `src/elf/elf32_loader.*` for validated mapping and `src/elf/elf32_dynamic.*` for structural raw dynamic entries.
- Android cross-build: `arm64-v8a`, NDK `27.3.13750724`.

D-0003 and D-0004 remain central: guest VAs are independent from host pointer identity, and high-base contiguous fastmem is the preferred first Android acceleration path when available.

## Repository map

- `src/cpu/`: engine adapter only.
- `src/memory/`: guest-memory contracts/backends.
- `src/elf/`: ELF mapping and structural metadata layers.
- `tests/`: host regression and real-fixture integration tests.
- `tools/`: Android probes/runtime-smoke and fixture tooling.
- `docs/architecture/`: subsystem boundaries and current design detail.
- `docs/research/`: research/evidence records.
- `specs/`: feature-scale requirements/design/tasks packages.
- `.agent/`: durable continuation state and decisions.

## Build / test entry points

Host tests:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DLIBA32ANDROID_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

GitHub Actions also builds the reproducible ARM32 Android fixture, cross-builds `liba32android.so` plus diagnostics for Android `arm64-v8a`, and publishes the relevant artifacts.

## Canonical specs / docs

- Repository workflow: `AGENTS.md`.
- Converted implemented baseline: `specs/000-current-baseline/`.
- Current observed state: `.agent/STATE.md`.
- Dependency-ordered next work: `.agent/NEXT.md`.
- Durable architecture decisions: `.agent/DECISIONS.md`.
- Detailed subsystem design/evidence: `docs/architecture/` and `docs/research/`.

For new feature-scale work, create a focused `specs/<id>-<feature>/requirements.md`, `design.md`, and `tasks.md` package before substantial implementation. Tiny/routine changes should not receive unnecessary spec ceremony.

## Evidence labels

Use `PASS`, `FAIL`, `BLOCKED`, and `NOT RUN` for execution status. Architecture/research prose may additionally distinguish observed facts from inference/hypothesis. Never turn inspection, inference, or an unexecuted test into PASS.
