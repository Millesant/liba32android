# Design — requester-aware dependency acquisition

## Provider API

`Elf32DependencyProvider` keeps the existing pure virtual
`resolve(requested_name, max_image_bytes)` contract.

An additive virtual `resolve_for(requester_identity, requested_name,
max_image_bytes)` defaults to calling `resolve`. This preserves all current
providers and gives future platform providers a narrow override point.

Both names are opaque byte strings. The generic layer performs no path
normalization, SONAME interpretation, namespace lookup, or UTF-8 assumptions.

## Borrowed requester lifetime

`Elf32DependencyResolveOptions` gains a `std::string_view
requester_identity`. The view is borrowed only for the synchronous
`resolve_elf32_dependencies` call and is never copied into successful output
or stored by the resolver.

The recursive loader sets it from
`graph.objects[index].identity` before provider invocation. Graph mutation
that may move object storage happens only after the resolver returns, so the
view remains valid for the entire provider call.

## Failure/resource semantics

Requester context changes no resource accounting or error mapping. Each
DT_NEEDED occurrence still consumes one ordered provider invocation, receives
the same per-image/remaining-total ceiling, and yields the same NotFound,
provider-failure, identity, image-size, and total-size validation.

Provider result identity remains the opaque graph deduplication key.

## Non-goals

No Android filesystem search, namespace accessibility/linking, RUNPATH/RPATH,
LD_LIBRARY_PATH, preload/RTLD policy, APK archive reader, platform library
catalog, shim generation, or symbol bridge is introduced.
