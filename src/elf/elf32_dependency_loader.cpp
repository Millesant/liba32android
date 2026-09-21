#include "elf/elf32_dependency_loader.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>

#include "elf/elf32_load_plan.h"

namespace liba32android::elf {
namespace {

[[nodiscard]] Elf32DependencyLoadResult failure(
    Elf32DependencyLoadError error,
    std::string failing_identity = {}) {
    Elf32DependencyLoadResult result;
    result.error = error;
    result.primary_error = error;
    result.failing_identity = std::move(failing_identity);
    return result;
}

[[nodiscard]] bool rollback_load(memory::MappedGuestMemory& memory,
                                 const Elf32LoadResult& load) {
    for (auto it = load.segments.rbegin(); it != load.segments.rend(); ++it) {
        if (it->mapping_size > std::numeric_limits<std::size_t>::max()) {
            return false;
        }
        if (!memory.unmap(it->mapping_start,
                          static_cast<std::size_t>(it->mapping_size))) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] Elf32DependencyLoadResult rollback_failure(
    memory::MappedGuestMemory& memory,
    const Elf32LoadResult& load,
    Elf32DependencyLoadResult result) {
    if (!rollback_load(memory, load)) {
        result.primary_error = result.error;
        result.error = Elf32DependencyLoadError::RollbackFailed;
    }
    result.graph = {};
    return result;
}

}  // namespace

Elf32DependencyLoadResult load_elf32_dependency_graph(
    memory::MappedGuestMemory& memory,
    Elf32DependencyLoadSource root,
    Elf32DependencyProvider& provider,
    const Elf32DependencyLoadOptions& options) {
    (void)provider;

    if (options.max_objects == 0) {
        return failure(Elf32DependencyLoadError::InvalidOptions, root.identity);
    }
    if (root.identity.empty()) {
        return failure(Elf32DependencyLoadError::EmptyRootIdentity);
    }
    if (root.image.empty()) {
        return failure(Elf32DependencyLoadError::EmptyRootImage, root.identity);
    }

    const std::uint64_t root_image_bytes =
        static_cast<std::uint64_t>(root.image.size());
    if (root_image_bytes > options.max_image_bytes) {
        return failure(Elf32DependencyLoadError::ImageTooLarge, root.identity);
    }
    if (root_image_bytes > options.max_total_image_bytes) {
        return failure(Elf32DependencyLoadError::TotalImageBytesExceeded,
                       root.identity);
    }

    const auto plan_result = plan_elf32_load(memory, root.image);
    if (!plan_result) {
        auto result = failure(Elf32DependencyLoadError::InvalidImage,
                              root.identity);
        result.load_error = plan_result.error;
        return result;
    }

    Elf32LoadResult load;
    if (plan_result.plan.type == Elf32ImageType::Dynamic) {
        const auto placement =
            place_elf32_dynamic(memory, root.image, options.placement);
        if (!placement) {
            auto result = failure(Elf32DependencyLoadError::PlacementFailed,
                                  root.identity);
            result.placement_error = placement.error;
            result.load_error = placement.load_error;
            return result;
        }

        Elf32LoadOptions load_options;
        load_options.dynamic_base = placement.dynamic_base;
        load = load_elf32(memory, root.image, load_options);
    } else {
        load = load_elf32(memory, root.image);
    }

    if (!load) {
        auto result = failure(Elf32DependencyLoadError::LoadFailed,
                              root.identity);
        result.load_error = load.error;
        return result;
    }

    Elf32LoadedDependencyObject object;
    object.identity = std::move(root.identity);
    object.image = std::move(root.image);
    object.load = load;

    if (load.dynamic_segment.has_value()) {
        const auto dynamic =
            parse_elf32_dynamic(memory, *load.dynamic_segment);
        if (!dynamic) {
            auto result =
                failure(Elf32DependencyLoadError::DynamicParseFailed,
                        object.identity);
            result.dynamic_error = dynamic.error;
            return rollback_failure(memory, load, std::move(result));
        }
        object.dynamic_entries = dynamic.entries;

        const auto metadata =
            build_elf32_linker_metadata(memory, load.load_bias,
                                        object.dynamic_entries);
        if (!metadata) {
            auto result =
                failure(Elf32DependencyLoadError::LinkerMetadataFailed,
                        object.identity);
            result.metadata_error = metadata.error;
            return rollback_failure(memory, load, std::move(result));
        }
        object.linker_metadata = metadata.metadata;

        const auto strings = build_elf32_linker_strings(
            memory, object.linker_metadata,
            Elf32LinkerStringOptions{
                .max_string_bytes = options.max_string_bytes,
            });
        if (!strings) {
            auto result =
                failure(Elf32DependencyLoadError::LinkerStringFailed,
                        object.identity);
            result.string_error = strings.error;
            return rollback_failure(memory, load, std::move(result));
        }
        object.linker_strings = strings.strings;

        const std::uint64_t direct_occurrences =
            static_cast<std::uint64_t>(object.linker_strings.needed.size());
        if (direct_occurrences > options.max_dependency_occurrences) {
            auto result =
                failure(Elf32DependencyLoadError::TooManyDependencyOccurrences,
                        object.identity);
            return rollback_failure(memory, load, std::move(result));
        }
        if (!object.linker_strings.needed.empty()) {
            auto result =
                failure(Elf32DependencyLoadError::DependenciesNotImplemented,
                        object.identity);
            result.requested_name = object.linker_strings.needed.front();
            return rollback_failure(memory, load, std::move(result));
        }
    }

    Elf32DependencyLoadResult result;
    result.graph.objects.push_back(std::move(object));
    return result;
}

const char* to_string(Elf32DependencyLoadError error) noexcept {
    switch (error) {
    case Elf32DependencyLoadError::None:
        return "none";
    case Elf32DependencyLoadError::InvalidOptions:
        return "invalid_options";
    case Elf32DependencyLoadError::EmptyRootIdentity:
        return "empty_root_identity";
    case Elf32DependencyLoadError::EmptyRootImage:
        return "empty_root_image";
    case Elf32DependencyLoadError::ImageTooLarge:
        return "image_too_large";
    case Elf32DependencyLoadError::TotalImageBytesExceeded:
        return "total_image_bytes_exceeded";
    case Elf32DependencyLoadError::TooManyObjects:
        return "too_many_objects";
    case Elf32DependencyLoadError::MaxDepthExceeded:
        return "max_depth_exceeded";
    case Elf32DependencyLoadError::TooManyDependencyOccurrences:
        return "too_many_dependency_occurrences";
    case Elf32DependencyLoadError::DependencyResolveFailed:
        return "dependency_resolve_failed";
    case Elf32DependencyLoadError::DependenciesNotImplemented:
        return "dependencies_not_implemented";
    case Elf32DependencyLoadError::IdentityImageMismatch:
        return "identity_image_mismatch";
    case Elf32DependencyLoadError::InvalidImage:
        return "invalid_image";
    case Elf32DependencyLoadError::DependencyNotDynamic:
        return "dependency_not_dynamic";
    case Elf32DependencyLoadError::PlacementFailed:
        return "placement_failed";
    case Elf32DependencyLoadError::LoadFailed:
        return "load_failed";
    case Elf32DependencyLoadError::DynamicParseFailed:
        return "dynamic_parse_failed";
    case Elf32DependencyLoadError::LinkerMetadataFailed:
        return "linker_metadata_failed";
    case Elf32DependencyLoadError::LinkerStringFailed:
        return "linker_string_failed";
    case Elf32DependencyLoadError::RollbackFailed:
        return "rollback_failed";
    }
    return "unknown";
}

}  // namespace liba32android::elf
