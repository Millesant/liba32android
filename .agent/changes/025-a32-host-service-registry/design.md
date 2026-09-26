# Design — A32 host-service registry

## Types

Add `A32HostServiceRegistryEntry` with:

- `std::uint32_t svc_immediate`;
- borrowed `A32HostServiceHandler* handler`.

Add `A32HostServiceRegistry final : A32HostServiceHandler` holding
`std::span<const A32HostServiceRegistryEntry>`.

The registry is itself compatible with feature 024 and therefore needs no
dispatcher changes.

## Dispatch algorithm

For one `handle` call:

1. scan the finite borrowed span;
2. skip entries whose immediate is not an exact match;
3. if a matching entry has a null child, points directly to this registry, or a
   previous match already exists, return `Failed` immediately;
4. after the scan, no match returns `Unhandled`;
5. otherwise delegate once to the matched child with the exact original
   `GuestMemory`, immediate, register array, and CPSR reference;
6. return the child disposition unchanged.

Duplicate matching IDs are detected before any child invocation because the
scan completes before delegation. Unrelated malformed entries remain ignored.

## Ownership and cost

The caller owns both the entry storage and child handlers for the full registry
lifetime. The registry allocates nothing. Lookup cost is linear in the supplied
finite span and therefore bounded by the caller-selected entry count.

## Layering

The implementation includes only runtime and generic memory contracts. It does
not interpret AAPCS registers or stack contents, perform guest string reads,
assign Android service IDs, generate/select ELF shims, or interact with
dependency/symbol/namespace policy.

## Verification

The dedicated runtime regression checks exact selection, unknown IDs, child
state/result forwarding, duplicate IDs, matching null handlers, unrelated
invalid entries, and an ARM `svc` -> registry -> dispatcher/resume path.
Exact-head required CI remains the acceptance gate.
