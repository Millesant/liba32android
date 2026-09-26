# Runtime spec delta — feature 025

Add a game-agnostic exact-SVC registry that composes caller-owned
`A32HostServiceHandler` objects and is itself usable as feature 024's handler.

The registry borrows a finite entry span. No match returns `Unhandled`.
Exactly one matching non-null child receives the unchanged `GuestMemory`,
exact SVC immediate, mutable register array, and mutable CPSR and returns its
normal disposition.

A matching null child, duplicate matching immediate, or direct self-entry
returns `Failed` before any child is invoked. Malformed entries for unrelated
immediates do not poison exact lookup. The registry owns no entry or handler
storage and performs no allocation.

No ABI argument decoding, guest-string handling, Android API semantics,
compatibility shim generation/selection, symbol policy, syscall behavior, or
namespace/search policy is added by this feature.
