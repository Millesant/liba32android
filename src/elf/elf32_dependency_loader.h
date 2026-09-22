#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "elf/elf32_dependency_resolver.h"
#include "elf/elf32_dynamic.h"
#include "elf/elf32_dynamic_placement.h"
#include "elf/elf32_linker_metadata.h"
#include "elf/elf32_linker_strings.h"
#include "elf/elf32_loader.h"
#include "memory/guest_memory.h"

namespace liba32android::elf {

struct Elf32DependencyLoadSource {
    std::string identity;
    std::vector<std::uint8_t> image;
};

struct Elf32DependencyLoadOptions {
    std::uint32_t max_objects{};
    std::uint32_t max_depth{};
    std::uint64_t max_dependency_occurrences{};
    std::uint64_t max_image_bytes{};
    std::uint64_t max_total_image_bytes{};
    std::uint32_t max_string_bytes{};
    Elf32DynamicPlacementOptions placement{};
};

struct Elf32DependencyEdge {
    std::string requested_name;
    std::size_t target_object{};
};

struct Elf32LoadedDependencyObject {
    std::string identity;
    std::vector<std::uint8_t> image;
    Elf32LoadResult load;
    std::vector<Elf32DynamicEntry> dynamic_entries;
    Elf32LinkerMetadata linker_metadata;
    Elf32LinkerStrings linker_strings;
    std::vector<Elf32DependencyEdge> dependencies;
};

struct Elf32DependencyGraph {
    std::vector<Elf32LoadedDependencyObject> objects;
};

enum class Elf32DependencyLoadError : std::uint8_t {
    None = 0,
    InvalidOptions,
    EmptyRootIdentity,
    EmptyRootImage,
    ImageTooLarge,
    TotalImageBytesExceeded,
    TooManyObjects,
    MaxDepthExceeded,
    TooManyDependencyOccurrences,
    DependencyResolveFailed,
    DependenciesNotImplemented,
    IdentityImageMismatch,
    InvalidImage,
    DependencyNotDynamic,
    PlacementFailed,
    LoadFailed,
    DynamicParseFailed,
    LinkerMetadataFailed,
    LinkerStringFailed,
    RollbackFailed,
};

struct Elf32DependencyLoadResult {
    Elf32DependencyLoadError error{Elf32DependencyLoadError::None};
    Elf32DependencyLoadError primary_error{Elf32DependencyLoadError::None};

    Elf32DependencyResolveError dependency_error{Elf32DependencyResolveError::None};
    Elf32DynamicPlacementError placement_error{Elf32DynamicPlacementError::None};
    Elf32LoadError load_error{Elf32LoadError::None};
    Elf32DynamicError dynamic_error{Elf32DynamicError::None};
    Elf32LinkerMetadataError metadata_error{Elf32LinkerMetadataError::None};
    Elf32LinkerStringError string_error{Elf32LinkerStringError::None};

    std::string failing_identity;
    std::string requested_name;
    Elf32DependencyGraph graph;

    [[nodiscard]] explicit operator bool() const noexcept {
        return error == Elf32DependencyLoadError::None;
    }
};

// Load a root plus its direct dependency set transactionally. T002 preserves
// ordered/repeated edges, reuses equal provider identities, and loads each
// first-seen dependency as ET_DYN. Recursive traversal of dependency objects'
// own DT_NEEDED entries is added by T003; encountering one currently fails
// explicitly and rolls back all graph-owned mappings.
[[nodiscard]] Elf32DependencyLoadResult load_elf32_dependency_graph(
    memory::MappedGuestMemory& memory,
    Elf32DependencyLoadSource root,
    Elf32DependencyProvider& provider,
    const Elf32DependencyLoadOptions& options);

[[nodiscard]] const char* to_string(Elf32DependencyLoadError error) noexcept;

}  // namespace liba32android::elf
