# Design — ELF32 lifecycle-array metadata

## Layering

Feature 017 stays below lifecycle orchestration. Linker metadata owns dynamic-tag
pairing, checked address rebasing, declared-range readability, and guest-only
descriptors. The lifecycle primitive owns bounded raw entry decoding. Neither
layer filters sentinel values or invokes guest code.

## Metadata invariants

- INIT_ARRAY and INIT_ARRAYSZ are a unique all-or-nothing pair.
- FINI_ARRAY and FINI_ARRAYSZ are a unique all-or-nothing pair.
- Sizes are byte counts divisible by the 4-byte ELF32 function-pointer width.
- Array addresses receive exactly one checked load-bias addition.
- The full declared range must fit the 32-bit guest address space and be
  readable through GuestMemory.
- Zero-length arrays remain representable even when their address is otherwise
  unreadable because no guest byte is consumed.

## Decoder invariants

The decoder accepts only a validated-style guest descriptor plus a caller
entry ceiling. It preflights size, ceiling, and address-space range before
entry reads, decodes little-endian 32-bit values in declaration order, and
returns no partial vector on read failure. Raw 0 and 0xffffffff values are
preserved for later Android lifecycle policy.

## Non-goals

No DT_INIT/DT_FINI, PREINIT_ARRAY, dependency ordering, sentinel filtering,
constructor-called recursion state, guest CPU invocation, argv/envp contract,
dlopen lifecycle, or unload/destructor orchestration is introduced.
