# ELF32 spec delta — feature 020

Extend dependency acquisition with optional requester context. Resolver options
may borrow the current loaded-object identity, and the provider receives that
identity byte-for-byte alongside each exact DT_NEEDED name.

The requester-aware provider hook is additive and defaults to the existing
context-free resolve method. Existing providers therefore remain behaviorally
unchanged. Recursive dependency loading supplies each currently processed
graph object's owned identity, including nested objects.

Requester identity is synchronous borrowed context only. Provider result
identity remains the opaque loaded-object key. Resource ceilings, error
translation, ordered/repeated occurrence semantics, and all-or-nothing output
are unchanged.

Android pathname/search/namespace policy, RUNPATH/RPATH, preload/RTLD behavior,
APK lookup, platform catalogs, and compatibility shims remain outside this
change.
