# A32 host-service registry

Status: feature 025 implementation prepared; exact-head validation NOT RUN

## Boundary

`src/runtime/a32_service_registry.*` is a game-agnostic composition layer above
feature 024's `A32HostServiceHandler` interface. It maps exact A32 SVC
immediates to caller-owned child handlers without interpreting guest ABI
arguments or adding platform behavior.

The registry borrows a finite
`std::span<const A32HostServiceRegistryEntry>`. Every entry contains an exact
SVC immediate and a borrowed `A32HostServiceHandler*`. The entry array and
handler objects must outlive the registry.

## Lookup semantics

For each trapped SVC:

1. scan only the supplied finite entry span;
2. no matching entry -> `Unhandled`;
3. exactly one matching non-null handler -> invoke it with the original
   `GuestMemory`, exact immediate, mutable registers, and mutable CPSR;
4. a matching null handler, duplicate matching immediate, or direct self-entry
   -> `Failed` before any child handler is invoked.

Malformed entries for other immediates do not poison an otherwise exact lookup.
The registry performs no allocation and does not own or reorder handlers.

The child handler's `Handled`, `Unhandled`, or `Failed` result is returned
unchanged. Register, CPSR, and guest-memory effects performed by an invoked
child follow feature 024's existing side-effect semantics.

## Layering

The registry contains no Dynarmic types and no Android API constants. It does
not decode AAPCS arguments, read guest strings, generate or select ELF shims,
resolve symbols, model syscalls, or choose namespace/search policy.

Those behaviors remain separate compatibility/platform layers. In particular,
this feature establishes only a deterministic service-number composition seam
for later VLC/FMOD-oriented compatibility shims.

## Validation scope

The feature-025 runtime regression directly covers unknown IDs, exact child
selection, child result/state forwarding, duplicate IDs, matching null
handlers, and unrelated malformed entries. It also executes an ARM
`svc #0x42; bx lr` program through feature 024 with the registry as the
dispatcher handler, proving the composition seam without claiming any Android
API behavior.
