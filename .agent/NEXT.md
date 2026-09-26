# Next Work

Repository integration is on `bleeding`. The current control-plane round is pinned to `millesant/.gpt@f4e926e81ad91d13d02a006f4a18a00f66ae0bab`.

`project-cleanup-v8` and its source/test, tooling, and documentation child changes are DONE. Features `011-elf32-combined-relocation-transaction` through `019-elf32-init-call-execution` are DONE. Feature 019 result revision `28a4f92f78b8ff156ee9dd3083d1218af8b7b125` passed exact-head Linux A32 smoke and both required Android checks.

Features 020 through 022 are DONE. Feature 022 result revision `4b91211abd005584992c4ddd75435c31a4ce0f2c` passed exact-head Linux A32 smoke and both required Android checks. The runtime can now compose requester-aware application/platform providers and exact-name caller-owned image catalogs while filesystem/APK/namespace policy remains external.

## Candidate runtime directions

Choose a bounded next feature from accepted gaps rather than continuing cosmetic repository churn. The supplied VLC ARMv7 four-DSO scan is recorded in `docs/research/evidence/vlc-armv7-gap-scan-2026-09-25.md`: every observed relocation is already inside the implemented REL/JUMP_SLOT set, while APK-external Android providers and INIT_ARRAY/FINI_ARRAY lifecycle metadata are concrete remaining requirements.

Current candidates include:

- layer Android namespace/search-path/platform-provider policy above the persistent link-map provider boundary;
- add a guest-to-host service-call seam for future Android compatibility shims, then use catalog-provided shim images to satisfy the supplied VLC/FMOD platform dependency names;
- broader ARM relocation coverage beyond REL32, such as COPY/instruction families or RELA/RELR/Android packed encodings, when a concrete target requires them;
- TLS/IFUNC groundwork when a concrete target requires it;
- end-to-end real ARM32 fixture execution on Android once the required runtime environment is available.

## Independent follow-ups

- Android native tombstone/backtrace coexistence: BLOCKED on accessible device environment.
- AArch64 runtime execution on a 16 KiB Android host: NOT RUN.
- Project license: BLOCKED on maintainer choice.
