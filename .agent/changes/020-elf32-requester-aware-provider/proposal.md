# Proposal — requester-aware ELF32 dependency provider

## Intent

Make the existing caller/platform dependency-provider boundary capable of
implementing requester-sensitive Android namespace/search policy without
putting that policy inside the generic ELF resolver.

The current provider receives only the requested DT_NEEDED name. Recursive
loading already knows the exact graph object whose dependencies are being
resolved, but that identity is discarded before provider invocation. Feature
020 carries it across the boundary.

## Why this slice now

The supplied VLC ARMv7 graph mixes APK-local libraries with Android platform
dependencies. Future namespace/provider policy must be able to choose
accessibility based on the requesting loaded object. Feature 016 supplies
persistent link-map lifetime, but the provider callback still lacks requester
context.

This feature is deliberately only the missing information seam. It does not
choose paths, namespaces, APK entries, platform libraries, or shims.

## Compatibility boundary

Existing providers implement `resolve(requested_name, max_image_bytes)`.
Feature 020 adds a virtual requester-aware hook whose default implementation
forwards to that method. Existing provider implementations therefore keep
their current behavior without edits.

Resolver options carry one borrowed requester identity for the duration of a
synchronous resolve call. The dependency loader sets that view from the
currently processed graph object's owned identity. No view is retained after
the call.
