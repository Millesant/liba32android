#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "elf/elf32_dependency_graph.h"

namespace liba32android::elf {

enum class Elf32LinkMapRootPolicy : std::uint8_t {
    Local = 0,
    Global,
};

struct Elf32LinkMapRoot {
    std::size_t object_index{};
    Elf32LinkMapRootPolicy policy{Elf32LinkMapRootPolicy::Local};
};

// Caller-owned persistent loaded-object registry. Object indexes are stable:
// successful appends only add objects and never reorder existing entries.
// roots/global_scope_objects contain indexes into graph.objects. Mutation is
// performed by the dependency-loader append API; callers may inspect the
// vectors directly but must preserve their index/dedup invariants.
struct Elf32LinkMap {
    Elf32DependencyGraph graph;
    std::vector<Elf32LinkMapRoot> roots;
    std::vector<std::size_t> global_scope_objects;

    [[nodiscard]] std::span<const std::size_t> global_scope() const noexcept {
        return global_scope_objects;
    }
};

}  // namespace liba32android::elf
