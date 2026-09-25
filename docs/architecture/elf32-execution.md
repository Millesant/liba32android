# Real ARM32 fixture execution

Status: feature 013 DONE — exact-head implementation CI PASSed at `e899822ae507d1d3954e670bef7365b3aa1196d4`

## Boundary

This integration layer proves that the existing ELF32 linker pipeline and A32 CPU adapter operate on one coherent guest address space. It remains a validation seam rather than a new public runtime orchestrator.

The pinned freestanding NDK fixture is loaded as one zero-dependency object. The test resolves its exported function/data symbols, applies the existing combined relocation transaction, initializes BSS, seals GNU RELRO, and executes `fixture_add`.

## Execution contract

The call harness uses logical guest state only:

- r0/r1 carry the two 32-bit function arguments;
- r13 points at a bounded mapped guest stack and is 8-byte aligned;
- r14 points at a mapped return sentinel;
- the instruction set and LR interworking bit follow the defining function symbol's Thumb discriminator;
- the CPU runs under a fixed 256-instruction budget.

The sentinel writes `0x5a` to r7 and loops. Successful validation therefore requires the exact r0 result, the return marker, no exception or memory fault, and the existing MappedGuestMemory fastmem capability.

## Validation evidence

At `e899822ae507d1d3954e670bef7365b3aa1196d4`, Linux A32 smoke check `108016545132` PASSed together with Android arm64-v8a cross-build check `108016544793` and Android x86_64 address-space probe check `108016545125`. The Linux workflow explicitly records and greps the execution evidence before uploading the fixture artifact.

## Limits

This does not prove Android-device execution, Android libc/JNI/graphics/audio compatibility, constructors/TLS, dlopen semantics, or the 16 KiB AArch64 Android host path.
