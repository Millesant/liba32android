# Next Work

Repository integration is on `bleeding`. Feature `006-elf32-symbol-resolution` is in T005 closeout.

1. T005 — documentation/state/spec convergence and final exact-head feature gate.
   - Status: ACTIVE.
   - Dependency gate: T004 PASSed exact-head CI #206 / run `35837480789` at `2fdba16a13e34370483701345de2605df06811e6`; Linux passed 41/41 CTest including `elf32_real_symbol_lookup`, Android x86_64 PASSed, and Android arm64-v8a PASSed.
   - Convergence scope: add the durable symbol-resolution architecture contract; reconcile README, project context/state, linker-metadata boundary documentation, requirements/tasks status, and acceptance evidence; preserve the explicit versioning/relocation/global-policy non-goals.
   - Requirements/design/code/test review: no unrecorded semantic gap was found that blocks R1-R17 / AC1-AC16.
   - Exact next action: run exact-head CI on the convergence commit and require Linux CTest, Android x86_64, and Android arm64-v8a to PASS.
   - On PASS: mark T005 and feature 006 DONE, persist the final gate identity, and leave the repository ready for the next feature-scale M4 slice.

2. Next M4 linker slice after feature 006.
   - Status: BLOCKED on T005 closeout.
   - ARM relocation application remains the nearest dependency-ready linker gap; it must begin with a new focused `specs/<id>-<feature>/` requirements/design/tasks package rather than being folded into feature 006.

Android native tombstone/backtrace evidence remains BLOCKED on an accessible device environment. Project license remains BLOCKED on maintainer choice.
