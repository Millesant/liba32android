# Design — provider-chain composition

## API

`Elf32DependencyProviderChain` implements `Elf32DependencyProvider` and
borrows a `std::span<Elf32DependencyProvider* const>`.

The span is the explicit finite provider set and order. Provider objects and
the span backing storage must outlive the chain.

Both `resolve` and `resolve_for` are implemented. Context-free resolution is
the same ordered algorithm with an empty requester identity. Requester-aware
resolution forwards the exact borrowed requester bytes to each child
`resolve_for`, allowing legacy providers to continue through feature 020's
default fallback.

## Result rules

For each provider:

- null pointer -> `Failed`;
- child `NotFound` -> try the next provider;
- child `Failed` -> return immediately;
- child success -> return immediately.

If no provider succeeds, return `NotFound`.

The chain does not validate image size, identity, or owned image contents;
those remain the existing dependency resolver's responsibility. It also does
not rewrite provider identity.

## Resource and lifetime boundary

The chain adds no independent byte accounting: the exact ceiling computed by
the resolver is forwarded unchanged to every attempted child. The provider list
is borrowed and never copied or persisted elsewhere.

## Non-goals

No pathname generation, package/archive access, namespace policy, Android
platform allowlists, shims, caching, preload order, or RTLD semantics.
