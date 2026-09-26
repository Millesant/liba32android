# Proposal — exact A32 host-service registry

## Intent

Add the smallest game-agnostic composition layer needed after feature 024:
map an exact trapped SVC immediate to one caller-owned
`A32HostServiceHandler` and let the existing bounded dispatcher execute it.

This feature deliberately stops before platform behavior. It creates a
deterministic service-number seam that later compatibility shims can target
without putting Android constants, ABI decoding, or application policy into the
CPU adapter or dispatcher.

## Registry boundary

The registry borrows a finite span of entries. Each entry contains:

- an exact 32-bit SVC immediate;
- a borrowed child `A32HostServiceHandler*`.

No entry or handler is owned, copied, allocated, or reordered by the registry.

## Lookup behavior

An unknown immediate is `Unhandled`. Exactly one matching non-null child is
called with the original memory/register/CPSR objects and exact immediate.
Duplicate matching IDs, a matching null handler, or a direct self-entry are
configuration failures and return `Failed` before child dispatch.

Malformed entries for other IDs do not affect a valid exact match.

## Validation

Add direct registry regression coverage plus one ARM SVC program executed
through `execute_a32_with_services` with the registry itself supplied as the
feature-024 handler.

## Non-goals

No Android API implementation, AAPCS argument/stack decoding, guest C-string
reader, shim ELF generation, dynamic-symbol policy, syscall ABI, platform
catalog, namespace/search behavior, JNI, graphics, or audio behavior.
