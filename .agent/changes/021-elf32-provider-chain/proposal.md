# Proposal — ordered ELF32 dependency provider chain

## Intent

Provide the smallest composition primitive needed to combine application-local
and platform dependency providers without teaching the generic ELF core how
Android paths or namespaces work.

Feature 020 now forwards requester identity to a provider. Feature 021 makes
that provider boundary composable: a caller supplies a finite ordered list of
providers, typically application-local first and platform fallback second.

## Semantics

- try providers in caller order;
- continue only after `NotFound`;
- return `Failed` immediately;
- return the first success immediately;
- forward requester identity, request bytes, and the image ceiling unchanged;
- empty chain returns `NotFound`;
- null entries are configuration failures, not silent skips.

The chain owns no providers and does not cache, deduplicate, normalize, or
reinterpret provider results.

## Non-goals

No Android filesystem search, namespace accessibility or links, RUNPATH/RPATH,
LD_LIBRARY_PATH, APK ZIP reading, preload/RTLD semantics, platform-library
catalog, compatibility shim selection, or symbol bridge.
