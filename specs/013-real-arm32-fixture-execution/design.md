# Design — real ARM32 fixture execution

Status: ACTIVE — implementation prepared; exact-head validation NOT RUN

## Pipeline

```text
generated ARM32 fixture
 -> dependency graph load
 -> graph-local lookup
 -> combined main+PLT relocation
 -> initialize fixture_bss
 -> GNU RELRO seal
 -> bounded cpu::execute(fixture_add)
 -> same-state return sentinel loop
```

The fixture is freestanding and zero-dependency. Its function accesses `fixture_data` and `fixture_bss`, so successful execution after relocation proves the CPU consumes the same guest mappings modified by the linker.

## Call harness

The test derives Thumb state from the raw defining `STT_FUNC` value. It strips bit zero from the execution entry PC, selects the matching CPU instruction set, and sets LR to a mapped return sentinel with the interworking bit restored for Thumb.

The sentinel first writes `0x5a` to r7 and then branches to itself. The CPU adapter already steps one guest instruction at a time under a fixed instruction ceiling, so reaching the marker proves the function returned without requiring a new CPU stop-condition API.

A small bounded RW guest stack is mapped independently. No host pointer is placed in guest registers.
