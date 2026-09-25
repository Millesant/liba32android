# Next Work

Repository integration is on `bleeding`. The current control-plane round is pinned to `millesant/.gpt@2b7c8b9245a64560cc9e986d554d99233e34b8c5`.

`project-cleanup-v8` and its source/test, tooling, and documentation child changes are DONE. Features `011-elf32-combined-relocation-transaction` and `012-elf32-rel32-relocation` are DONE; REL32 implementation revision `ee4d2b364244fd842b059dd5c254de01709c65f9` passed Linux A32 smoke and both Android required checks.

No new runtime feature-scale implementation is selected.

## Candidate runtime directions

Choose a bounded next feature from accepted gaps rather than continuing cosmetic repository churn. Current candidates include:

- Android search-path/namespace/link-map lifetime policy above the provider boundary;
- version-aware symbol resolution/interposition;
- broader ARM relocation coverage beyond REL32, such as COPY/instruction families or RELA/RELR/Android packed encodings;
- TLS/IFUNC groundwork;
- end-to-end real ARM32 fixture execution on Android once the required runtime environment is available.

## Independent follow-ups

- Android native tombstone/backtrace coexistence: BLOCKED on accessible device environment.
- AArch64 runtime execution on a 16 KiB Android host: NOT RUN.
- Project license: BLOCKED on maintainer choice.
