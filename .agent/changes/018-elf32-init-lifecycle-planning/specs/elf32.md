# ELF32 spec delta — feature 018

Add a root-scoped read-only INIT_ARRAY planning contract above feature 017.
Reachable dependencies are planned before the requester in deterministic stored
edge order. Cycles and shared dependencies contribute each object at most once.

The caller bounds both unique objects visited and total raw INIT_ARRAY entries
decoded. Null and all-ones entries are counted for resource purposes but
filtered from the final call list. Every retained call records object index,
array index, and raw logical function value.

Failure yields no partial plan and performs no guest or graph mutation. Legacy
DT_INIT, PREINIT_ARRAY, destructor planning, CPU execution, persisted lifecycle
state, dlopen, and unload remain deferred.
