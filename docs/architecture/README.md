# Architecture index

The runtime is intentionally layered. Dependencies should flow toward lower-level contracts rather than sideways into implementation details.

```text
ARM ELF32 image
  -> loading
  -> structural metadata
  -> linker metadata + strings
  -> dependency graph
  -> symbol lookup
  -> relocation
  -> RELRO hardening

GuestMemory
  -> CPU adapter
  -> Dynarmic (internal implementation detail)
```

Implementation files are grouped under `src/elf/loading/`, `metadata/`, `linking/`, and `hardening/`. ELF interface headers remain at `src/elf/` so layer contracts are easy to discover and include paths stay stable within the repository.

Each architecture document describes one boundary and should avoid folding later-layer policy into earlier layers.
