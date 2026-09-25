# Design — ELF32 ARM R_ARM_REL32 relocation

Status: DONE — exact-head implementation CI PASSed at `ee4d2b364244fd842b059dd5c254de01709c65f9`

AAELF32 defines type 3 as `((S + A) | T) - P`. The relocation reference now retains additive defining-symbol metadata because `T` belongs to the defining target symbol, not necessarily the request symbol. For a defining Thumb `STT_FUNC`, the implementation strips bit zero from relocation `S`, sets `T=1`, computes the wrapped result, and feeds it into the existing precomputed pending-write transaction. Unresolved weak references have no defining symbol and use `S=0,T=0`.

No transaction boundary, permission behavior, or symbol-lookup policy changed.
