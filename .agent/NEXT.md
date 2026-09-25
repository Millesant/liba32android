# Next Work

Repository integration is on `bleeding`. The current control-plane round is pinned to `millesant/.gpt@2b7c8b9245a64560cc9e986d554d99233e34b8c5`.

`project-cleanup-v8` and its source/test, tooling, and documentation child changes are DONE. Features `011-elf32-combined-relocation-transaction`, `012-elf32-rel32-relocation`, `013-real-arm32-fixture-execution`, and `014-elf32-symbol-versioning` are DONE; feature 014 implementation revision `5ba659dbf3ad9328c8e46af4441db3a0c4bb4a26` passed Linux A32 smoke and both Android required checks.

No new runtime feature-scale implementation is selected. Host execution of the freestanding fixture is now proven, so the next runtime feature should target semantics needed by non-freestanding Android libraries rather than more integration plumbing.

## Candidate runtime directions

Choose a bounded next feature from accepted gaps rather than continuing cosmetic repository churn. Current candidates include:

- Android search-path/namespace/link-map lifetime policy above the provider boundary;
- DT_SYMBOLIC/requester-specific self-binding plus Android/global-group interposition policy;
- broader ARM relocation coverage beyond REL32, such as COPY/instruction families or RELA/RELR/Android packed encodings;
- TLS/IFUNC groundwork;
- end-to-end real ARM32 fixture execution on Android once the required runtime environment is available.

## Independent follow-ups

- Android native tombstone/backtrace coexistence: BLOCKED on accessible device environment.
- AArch64 runtime execution on a 16 KiB Android host: NOT RUN.
- Project license: BLOCKED on maintainer choice.
