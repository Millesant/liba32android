# Requirements — ELF32 ARM R_ARM_REL32 relocation

Status: DONE — exact-head implementation CI PASSed at `ee4d2b364244fd842b059dd5c254de01709c65f9`

## Goal

Add `R_ARM_REL32` (type 3) to the main ARM ELF32 `DT_REL` relocation path.

## Accepted requirements

- Main REL accepts type 3; PLT REL continues to accept only JUMP_SLOT.
- REL32 is symbol-bearing and uses the existing bounded reference validation/graph lookup contract.
- The result is `((S + A) | T) - P` modulo 2^32.
- `P` is the checked logical guest address of the relocation target.
- For a defining Thumb `STT_FUNC`, relocation `S` strips the raw symbol-value Thumb discriminator and `T=1`; otherwise `T=0`.
- An unresolved weak reference uses `S=0,T=0`.
- Planning/resolution/final-word computation completes before writes; failures retain existing reverse-rollback behavior.
- No host pointers or permission changes are introduced.

## Validation

Linux check `108000309377`, Android arm64 check `108000309663`, and Android x86_64 check `108000309608` PASSed on the implementation revision.
