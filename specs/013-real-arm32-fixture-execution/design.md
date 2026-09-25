# Design — real ARM32 fixture execution

Status: DONE — exact-head implementation CI PASSed at `e899822ae507d1d3954e670bef7365b3aa1196d4`

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

The harness derives Thumb state from the raw defining `STT_FUNC` value, strips bit zero from the execution entry PC, selects the matching CPU instruction set, and sets LR to a mapped return sentinel with the Thumb interworking bit restored when needed. The sentinel writes `0x5a` to r7 and loops, allowing the fixed-budget CPU adapter to prove return without a new stop-condition API.

A bounded RW guest stack is mapped independently and kept 8-byte aligned. No host pointer is placed in guest registers.
