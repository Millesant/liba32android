# Next Work

Repository integration is on `bleeding`. Features `006-elf32-symbol-resolution`, `007-elf32-relocations`, `008-elf32-plt-relocation-metadata`, and `009-elf32-jump-slot-relocations` are DONE. Feature 009's final exact-head gate PASSed CI #229 / run `35940125841` at `4b255695a9effbaab4028708cd5e7e5a5e23150e`.

1. Next M4 linker/runtime slice — READY FOR SELECTION.
   - Preserve completed feature-009 behavior: separate bounded main-REL and PLT-REL APIs; eager PLT supports only `R_ARM_JUMP_SLOT = S`, shares graph-local symbol policy/rollback, and does not execute guest code.
   - Deferred areas include lazy binding/`DT_PLTGOT`, version-aware/protected requester semantics, Android/global-group/process-wide interposition policy, RELRO, TLS, IFUNC, combined main+PLT atomic application, and broader relocation forms. This list is not a priority ordering.
   - Exact next action: select one bounded M4 slice, establish its acceptance/evidence requirements, and readiness-check a new focused spec before implementation.

Independent FOLLOW_UP items remain unchanged:

- Android native tombstone/backtrace coexistence: BLOCKED on accessible device environment.
- AArch64 runtime execution on a 16 KiB Android host: NOT RUN.
- Project license: BLOCKED on maintainer choice.

Do not fold these independent items into the next M4 feature unless its accepted scope explicitly requires them.
