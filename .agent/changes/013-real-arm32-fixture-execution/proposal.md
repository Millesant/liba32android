# Proposal — real ARM32 fixture execution

## Intent

Close the next integration evidence gap by executing the existing pinned freestanding ARM32 loader fixture after the already-implemented load, symbol, relocation, and GNU RELRO stages.

## Why this slice now

The project can already execute synthetic A32 instructions and can independently load/link/relocate/harden the real NDK fixture. The smallest coherent continuation is to join those proven seams before adding more speculative linker families or Android compatibility layers.

## Boundary

This is host-side integrated runtime validation. It does not add Android libc/JNI/graphics/audio shims, constructors, TLS, dlopen semantics, or device execution. Android/AArch64 16 KiB runtime execution remains a separate external-evidence milestone.
