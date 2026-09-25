# Requirements — ELF32 ARM R_ARM_REL32 relocation

Status: ACTIVE — implementation prepared; exact-head validation NOT RUN

## Goal

Add `R_ARM_REL32` (type 3) to the main ARM ELF32 `DT_REL` relocation path.

## Requirements

- Main REL accepts type 3; PLT REL continues to accept only JUMP_SLOT.
- REL32 is symbol-bearing and uses the existing bounded reference validation/graph lookup contract.
- The result is `((S + A) | T) - P` modulo 2^32.
- `P` is the checked logical guest address of the relocation target.
- For a defining Thumb `STT_FUNC`, relocation `S` strips the raw symbol-value Thumb discriminator and `T=1`; otherwise `T=0`.
- An unresolved weak reference uses `S=0,T=0`.
- Planning/resolution/final-word computation completes before writes; failures retain existing reverse-rollback behavior.
- No host pointers or permission changes are introduced.

## Acceptance

Synthetic tests cover ordinary ARM data, a Thumb-function symbol with an addend that distinguishes correct T handling, and unresolved weak behavior. Final exact-head Linux/Android CI must pass before closure.
