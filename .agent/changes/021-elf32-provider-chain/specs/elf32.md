# ELF32 spec delta — feature 021

Add a caller-owned ordered dependency-provider chain above feature 020.
Providers receive the exact requester identity, request bytes, and current
image ceiling. The chain falls through only on `NotFound`; a hard provider
failure or first success terminates lookup immediately.

An empty chain reports `NotFound`. Null entries are configuration failures.
The chain owns no provider and borrows its finite ordered provider list.

Resolver validation, provider-result identity, image ownership, and byte
accounting remain unchanged. Android pathname/search/namespace policy and
concrete platform-provider catalogs remain outside this feature.
