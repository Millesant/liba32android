# Next Work

Repository integration is on `bleeding`. Feature `006-elf32-symbol-resolution` is DONE. Feature `007-elf32-relocations` is in T005 closeout.

1. T005 — documentation/state/spec convergence and final exact-head feature gate.
   - Status: READY.
   - Dependency gate: T004 PASSed exact-head CI #216 / run `35891830738` at `5d74af22c16a7bc99eee7038dfb9f137b22807c2`; Linux passed 45/45 CTest including `elf32_real_relocation_apply`, Android x86_64 PASSed, and Android arm64-v8a PASSed.
   - Convergence scope: add the durable ELF32 relocation architecture contract; reconcile README, project context/state, linker/symbol boundary docs, requirements/tasks status, and final validation evidence; preserve explicit PLT/JMPREL/versioning/TLS/IFUNC/global-policy/RELRO non-goals.
   - Exact next action in a fresh bounded round: compare R1-R15 / AC1-AC15 against implementation/tests, update required docs/state/spec, and run final exact-head CI.
   - On PASS: mark T005 and feature 007 DONE, persist the final gate identity, and leave the repository ready for the next M4 linker/runtime slice.

Independent FOLLOW_UP items remain unchanged:

- Android native tombstone/backtrace coexistence: BLOCKED on accessible device environment.
- AArch64 runtime execution on a 16 KiB Android host: NOT RUN.
- Project license: BLOCKED on maintainer choice.

Do not fold these independent items into feature 007 convergence.
