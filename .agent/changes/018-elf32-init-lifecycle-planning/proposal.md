# Proposal — ELF32 INIT_ARRAY lifecycle planning

## Intent

Turn feature 017's validated raw INIT_ARRAY entries into a deterministic,
non-executing constructor call plan for one dependency-graph root.

The planner follows Android's dependency-before-requester constructor shape,
suppresses repeated/cyclic object visits, filters null/all-ones call sentinels,
and remains fully read-only. CPU invocation is deliberately deferred.

## Why this slice now

The supplied VLC ARMv7 set demonstrates real INIT_ARRAY metadata, while feature
016 now provides persistent loaded-object identity and feature 017 provides
validated bounded array decoding. Planning is the smallest coherent bridge
between those prerequisites and later constructor execution.

## Boundary

- graph-root input plus caller object/entry ceilings;
- dependency-first DFS in stored edge order;
- one contribution per reachable object despite cycles/shared dependencies;
- raw feature-017 decoding for each object's INIT_ARRAY;
- filter 0 and 0xffffffff only when producing planned calls;
- retain object index, array entry index, and raw function value.

No DT_INIT, PREINIT_ARRAY, FINI_ARRAY planning, CPU execution, persisted
constructor-called state, dlopen lifecycle, unload, argv/envp modeling, or
platform-provider policy is introduced.
