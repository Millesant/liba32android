# Compatibility spec delta — feature 028

Add a requester-aware Android platform provider for concrete compatibility
libraries. Feature 028 recognizes only exact `liblog.so`.

For that name, forward requester identity and requested-name bytes unchanged to
a caller-owned policy returning Allow, NotFound, or Failed. Allow reuses the
existing exact-name catalog provider; NotFound/Failed publish no source. Unknown
names remain NotFound without policy invocation. Context-free resolve uses an
empty requester identity.

The provider owns one catalog entry and borrows the policy plus generated shim
bytes. It is non-copyable/non-movable because the nested catalog span points
into its own entry storage.

Prove application-provider NotFound -> platform-provider fallback through the
existing generic chain, including a real ARM32 consumer request whose
`android-log-consumer` identity reaches platform policy unchanged before
liblog loading/relocation/execution.

Do not define Android namespace names, linked namespaces, path/search rules,
public-library allowlists, preload/RTLD behavior, or automatic provider
installation.
