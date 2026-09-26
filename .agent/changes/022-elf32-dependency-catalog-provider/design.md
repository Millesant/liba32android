# Design — dependency catalog provider

## API

`Elf32DependencyCatalogEntry` contains borrowed:

- `requested_name` bytes;
- opaque `identity` bytes;
- ELF `image` bytes.

`Elf32DependencyCatalogProvider` borrows a finite
`std::span<const Elf32DependencyCatalogEntry>` and implements the existing
context-free provider method. Feature 020's default `resolve_for` therefore
makes catalog providers requester-compatible without interpreting requester
identity.

## Lookup

The provider linearly scans the finite catalog because this layer optimizes for
small explicit caller catalogs and deterministic behavior, not global caching.

For the requested byte string:

1. find exact byte-equal entries;
2. zero matches -> NotFound;
3. multiple matches -> Failed;
4. one match -> validate non-empty identity/image and image size <= supplied
   ceiling;
5. copy identity and image into an owned provider result.

No unrelated malformed entry poisons lookup for another name; validation is
performed on the unique selected entry.

## Composition

Two catalogs can be placed in an `Elf32DependencyProviderChain`, e.g.
application-local first and platform second. The first catalog's NotFound
continues to the next catalog; malformed/ambiguous matches fail hard.

## Non-goals

No pathname construction or normalization, archive reads, namespace policy,
platform catalog contents, shims, or caching.
