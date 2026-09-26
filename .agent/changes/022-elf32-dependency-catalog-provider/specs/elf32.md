# ELF32 spec delta — feature 022

Add an exact-name caller-owned dependency catalog provider. Catalog entries
borrow request-name, provider-identity, and ELF-image bytes. Lookup is exact
byte equality with no normalization. A unique valid match returns owned
identity/image bytes; no match reports NotFound; duplicate exact matches,
empty selected identity/image, or a selected image exceeding the supplied
provider ceiling report provider failure.

The provider is requester-agnostic and inherits feature 020's default
requester-aware fallback. It composes directly through the feature-021 ordered
provider chain. Resolver validation and aggregate byte accounting remain
unchanged.

Filesystem/APK access, Android namespaces/search paths, platform catalog
selection, preload/RTLD semantics, and shims remain deferred.
