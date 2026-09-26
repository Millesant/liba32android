# Next Work

Repository integration is on `bleeding`. The current control-plane round is pinned to `millesant/.gpt@f4e926e81ad91d13d02a006f4a18a00f66ae0bab`.

`project-cleanup-v8` and its source/test, tooling, and documentation child changes are DONE. Features `011-elf32-combined-relocation-transaction` through `019-elf32-init-call-execution` are DONE. Feature 019 result revision `28a4f92f78b8ff156ee9dd3083d1218af8b7b125` passed exact-head Linux A32 smoke and both required Android checks.

Features 020 through 024 are DONE. Feature 024 result revision `d4e7b480e28d13e8edc5dd1ccf28abbc3072a7a1` passed exact-head Linux A32 smoke and both required Android checks. The runtime now has requester-aware provider composition, exact-name image catalogs, resumable SVC traps, and a bounded game-agnostic host-service dispatcher.

Repository maintenance change `project-cleanup-v9` is ACTIVE. It audits canonical state/current docs against source, CMake registration, and exact-head evidence before the next runtime feature.

## Candidate runtime directions

Choose a bounded next feature from accepted gaps rather than continuing cosmetic repository churn. The supplied VLC ARMv7 four-DSO scan is recorded in `docs/research/evidence/vlc-armv7-gap-scan-2026-09-25.md`: every observed relocation is already inside the implemented REL/JUMP_SLOT set, while APK-external Android providers and INIT_ARRAY/FINI_ARRAY lifecycle metadata are concrete remaining requirements.

Current candidates include:

- layer Android namespace/search-path/platform-provider policy above the persistent link-map provider boundary;
- generate catalog-provided ARM32 shim stubs for the highest-priority VLC/FMOD platform APIs on top of the verified host-service dispatcher;
- broader ARM relocation coverage beyond REL32, such as COPY/instruction families or RELA/RELR/Android packed encodings, when a concrete target requires them;
- TLS/IFUNC groundwork when a concrete target requires it;
- end-to-end real ARM32 fixture execution on Android once the required runtime environment is available.

## Independent follow-ups

- Android native tombstone/backtrace coexistence: BLOCKED on accessible device environment.
- AArch64 runtime execution on a 16 KiB Android host: NOT RUN.
- Project license: BLOCKED on maintainer choice.
