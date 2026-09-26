# Proposal — explicit Android platform provider policy

## Intent

Add the smallest concrete platform-library selection boundary now that the
generated `liblog.so` shim exists.

Do not implement Android namespaces yet. Instead, make the platform fallback
explicit and requester-aware so future namespace/accessibility logic has one
narrow policy seam rather than being embedded into the generic ELF provider
chain.

## Scope

`A32AndroidPlatformProvider` implements `Elf32DependencyProvider` and
recognizes only `liblog.so`.

For that exact name it forwards the requester identity and requested name to a
caller-owned access policy. Allow serves the feature-027 shim through existing
catalog semantics. NotFound and Failed preserve generic provider-chain
behavior.

Unknown names bypass policy and return NotFound.

## Composition

Keep application providers first. Put the Android platform provider later in
`Elf32DependencyProviderChain`. The existing chain is still responsible for
fallthrough and exact requester forwarding.

Switch the real ARM32 log-shim integration to this composition and assert the
root identity reaches platform policy unchanged.

## Non-goals

No namespace/link graph, search paths, filesystem/APK lookup, public library
allowlist, environment/preload/RTLD rules, or automatic platform-provider
installation.
