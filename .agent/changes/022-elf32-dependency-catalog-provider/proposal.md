# Proposal — exact-name ELF32 dependency catalog provider

## Intent

Add the first concrete dependency provider above the generic requester/chain
seams: a finite caller-owned in-memory catalog mapping exact DT_NEEDED bytes to
an opaque provider identity and ELF image bytes.

The catalog is intentionally storage-policy-free. It can represent extracted
APK-local libraries, preloaded test images, or a caller-maintained platform
library set, and multiple catalogs can be ordered through feature 021.

## Semantics

- exact byte-name match only;
- no match -> NotFound;
- more than one exact match -> Failed;
- matching empty identity/image -> Failed;
- matching image larger than the supplied provider ceiling -> Failed before
  copying;
- unique valid match -> owned identity/image copy.

Catalog entries and their byte backing are borrowed and must outlive the
provider. Successful provider results are owned and independent of that
storage.

## Non-goals

No filesystem search, APK reader, namespace graph, RUNPATH/RPATH,
LD_LIBRARY_PATH, preload/RTLD semantics, Android platform allowlist, shims, or
requester-based filtering.
