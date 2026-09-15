# CPU engine architecture

Status: Accepted for M0

## Boundary

The CPU engine is an internal service. It executes AArch32 instructions and exposes guest CPU state/memory interaction to the rest of the runtime. It must not know about ELF dependency resolution, Android APIs, JNI, graphics, audio, Minecraft, or application profiles.

Dependency direction is intentionally one-way:

```text
runtime / loader / ABI layers
          |
          v
   CPU engine adapter
          |
          v
       Dynarmic
```

Application-specific code may depend on the generic runtime, never the reverse.

## Selected engine

M0 uses Dynarmic, pinned to `azahar-emu/dynarmic` commit `e77b1ba0b7da7cbe93021b01a663acfe7c4dd516` (2026-06-24).

The selection is based on directly observed upstream properties:

- A32 guest frontend including ARMv7 and Thumb/Thumb-2 families.
- AArch64 host backend.
- Android listed as a supported host OS.
- Bring-your-own-memory callbacks plus page-table and 4 GiB fastmem hooks.
- Cache invalidation API and configurable code cache.
- C++20 embedding API.
- 0BSD license for Dynarmic itself.
- The selected fork had a verified upstream commit in June 2026 and is used by the actively maintained Azahar emulator tree.

## M0 integration

`src/cpu/dynarmic_cpu.*` is deliberately small. It owns the Dynarmic-specific callback implementation and prevents Dynarmic types from leaking upward into future ELF/linker/ABI modules.

The M0 scratch-memory implementation is callback-based and only 4 KiB. It is not the future guest address-space design and must not be mistaken for one.

## Future memory design

Dynarmic exposes both a page table and `fastmem_pointer`, where fastmem models a contiguous 4 GiB guest address space. This is relevant to the low-VA/direct-address hypothesis from reverse-engineering research, but no low-address reservation strategy is accepted yet. Android mapping restrictions, ASLR collisions, executable permissions, guards and fallback behavior must be measured before M2 chooses a memory model.

## Correctness policy

Dynarmic documents known accuracy tradeoffs and is not treated as a formal ARM reference implementation. Tiny regression binaries and, where practical, a slower reference path will be used to verify runtime behavior. Unsupported behavior must be surfaced rather than silently declared compatible.

## Sources

- https://github.com/azahar-emu/dynarmic/tree/e77b1ba0b7da7cbe93021b01a663acfe7c4dd516
- https://github.com/azahar-emu/dynarmic/blob/e77b1ba0b7da7cbe93021b01a663acfe7c4dd516/README.md
- https://github.com/azahar-emu/dynarmic/blob/e77b1ba0b7da7cbe93021b01a663acfe7c4dd516/src/dynarmic/interface/A32/config.h
- https://github.com/azahar-emu/dynarmic/blob/e77b1ba0b7da7cbe93021b01a663acfe7c4dd516/src/dynarmic/interface/A32/a32.h
