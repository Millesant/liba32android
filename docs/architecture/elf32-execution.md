# Real ARM32 fixture execution

Status: feature 013 implementation prepared; exact-head validation NOT RUN

## Boundary

This integration layer proves that the existing ELF32 linker pipeline and A32 CPU adapter operate on one coherent guest address space. It is intentionally a validation seam rather than a new public runtime orchestrator.

The pinned freestanding NDK fixture is loaded as one zero-dependency object. The test resolves its exported function/data symbols, applies the existing combined relocation transaction, initializes BSS, seals GNU RELRO, and then executes `fixture_add`.

## Execution contract

The call harness uses logical guest state only:

- r0/r1 carry the two 32-bit function arguments;
- r13 points at a bounded mapped guest stack and is 8-byte aligned;
- r14 points at a mapped return sentinel;
- the instruction set and LR interworking bit follow the defining function symbol's Thumb discriminator;
- the CPU runs under a fixed instruction budget.

The sentinel writes a marker to r7 and loops. Therefore successful validation requires both the expected r0 return value and the marker, with no exception or memory fault.

## What this proves

A passing test demonstrates, on the Linux validation host, that generated ARM32 code can consume linker-relocated guest data through the same `MappedGuestMemory` instance used by Dynarmic and return through ordinary AAPCS/interworking state.

It does not prove Android-device execution, Android libc/JNI/graphics/audio compatibility, constructors/TLS, dlopen semantics, or the 16 KiB AArch64 Android host path.
