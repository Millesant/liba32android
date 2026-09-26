# Repository truth audit — 2026-09-26

Audited base revision:
`f45da9d530524a2701b053e84c36f5860e607582`

## Canonical-state drift

- feature 024 had exact-head green implementation evidence but was still
  described as ACTIVE/pending in current state before its closure commit;
- `.agent/STATE.md` still named control revision
  `2b7c8b9245a64560cc9e986d554d99233e34b8c5` instead of the pinned current
  round `f4e926e81ad91d13d02a006f4a18a00f66ae0bab`;
- the phase summary stopped at dependency-first INIT_ARRAY planning even though
  features 019-024 are complete;
- `.agent/NEXT.md` still named lifecycle metadata as a remaining requirement.

## Current-document drift

Current docs contained layer descriptions written before downstream work landed:

- `docs/architecture/elf32-dynamic.md`: "below any future dynamic linker";
- `docs/architecture/elf32-loader.md`: diagram/text still used "future linker"
  and described recursive graph ownership as a later layer;
- `docs/architecture/elf32-dependency-resolution.md`: deliberate-limits text
  still said automatic placement was available to a "future graph/loader"
  despite the implemented dependency loader/link map;
- `docs/architecture/elf32-linker-metadata.md`: a "does not yet" list mixes
  layer-local ownership boundaries with repo-wide gaps and needs an explicit
  scope warning;
- `docs/diagnostics.md`: "public runtime API is not implemented" is still
  true for a stable embedding API, but needs to distinguish that from the
  implemented internal runtime/service dispatcher.

## Historical-spec trap

The root `specs/` tree is explicitly historical. Feature 010 still contains
the original ACTIVE / exact-head-NOT-RUN text, while current RELRO architecture
and canonical state record feature 010 complete. Historical bodies should not
be rewritten, but the root index should call out that embedded status lines are
snapshots, not current project truth.

## Implementation/test hygiene

Bounded repository searches found:

- TODO: 0
- FIXME: 0
- placeholder: 0
- DISABLED_: 0
- GTEST_SKIP: 0

A complete non-truncated Git tree / CMake registration comparison found:

- source `.cpp` files under `src/`: 17
- test `.cpp` files under `tests/`: 35
- unreferenced source `.cpp` files: 0
- unreferenced test `.cpp` files: 0
- current test executable identities registered by CMake: 35
- current unique CTest names: 55
- duplicate test executable names: 0
- duplicate CTest names: 0

This does not prove semantic completeness. Genuine unsupported/deferred
semantics remain explicitly tracked in `.agent/STATE.md`; the purpose of this
audit is to remove accidental ambiguity between those accepted gaps and stale
documentation.


## Additional stale-current-doc finding

A second architecture pass found
`docs/architecture/elf32-linker-strings.md` still describing graph/mapping as
future work and claiming a final exact-head gate remained outstanding. The
historical feature-002 task record shows T005 DONE at CI #106, and downstream
dependency loading/link-map/provider work is implemented. Cleanup v9 updates
that current architecture page while leaving the historical numbered spec body
unchanged.


## Layer-local non-goal ambiguity

The audit also found current symbol, relocation, RELRO, and dependency-loading
architecture pages whose "does not implement" lists mixed true repository gaps
with behavior that was already implemented in downstream layers. Cleanup v9
rewrites those sections as explicit layer-local ownership boundaries and names
which downstream capabilities are already implemented versus genuinely
deferred.
