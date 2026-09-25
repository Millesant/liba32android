# Requirements — real ARM32 fixture execution

Status: ACTIVE — implementation prepared; exact-head validation NOT RUN

## Goal

Execute the pinned real ARM32 fixture's exported `fixture_add` through the generic CPU adapter after normal dependency loading, relocation, and GNU RELRO sealing.

## Requirements

- Reuse the current generated NDK r27d/API 26 fixture; do not introduce an opaque binary.
- Resolve `fixture_add`, `fixture_data`, and `fixture_bss` through the accepted graph-local symbol path.
- Apply the existing combined relocation transaction before execution and seal GNU RELRO afterward.
- Initialize `fixture_bss` through guest memory and verify `fixture_add(lhs,rhs)` observes both relocated data symbols.
- Derive ARM/Thumb execution state from the defining function symbol.
- Provide AAPCS arguments in r0/r1, a mapped 8-byte-aligned guest stack, and a same-state mapped return sentinel.
- Bound execution by a fixed instruction ceiling; successful return must reach the sentinel marker without exception or memory fault.
- Keep all addresses logical 32-bit guest VAs and all memory access behind `GuestMemory`.
- Preserve the zero-dependency provider invariant.
- Do not claim Android-device execution from a Linux-host execution result.

## Acceptance

Linux CTest and explicit fixture evidence must report the expected function result, return marker, relocation count, and PASS status. Required Android cross-build/probe lanes must remain green.
