#include "elf/elf32_dependency_loader.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <vector>

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
    bool success = true;
    for (auto it = load.segments.rbegin(); it != load.segments.rend(); ++it) {
        if (it->mapping_size > std::numeric_limits<std::size_t>::max()) {
            success = false;
            continue;
        }
        if (!memory.unmap(it->mapping_start,
                          static_cast<std::size_t>(it->mapping_size))) {
            success = false;
        }
    }
    return success;
}

[[nodiscard]] Elf32DependencyLoadResult rollback_failure(
    memory::MappedGuestMemory& memory,
    const std::vector<Elf32LoadResult>& successful_loads,
    Elf32DependencyLoadResult result) {
    bool rollback_ok = true;
    for (auto it = successful_loads.rbegin();
         it != successful_loads.rend(); ++it) {
        if (!rollback_load(memory, *it)) {
            rollback_ok = false;
        }
    }
    if (!rollback_ok) {
        result.primary_error = result.error;
        result.error = Elf32DependencyLoadError::RollbackFailed;
    }
    result.graph = {};
    return result;
}

[[nodiscard]] Elf32DependencyLoadResult load_object(
    memory::MappedGuestMemory& memory,
    Elf32DependencyLoadSource source,
    const Elf32DependencyLoadOptions& options,
    bool require_dynamic,
    Elf32LoadedDependencyObject& object) {
    const auto plan_result = plan_elf32_load(memory, source.image);
    if (!plan_result) {
        auto result =
            failure(Elf32DependencyLoadError::InvalidImage, source.identity);
        result.load_error = plan_result.error;
        return result;
    }
    if (require_dynamic && plan_result.plan.type != Elf32ImageType::Dynamic) {
        return failure(Elf32DependencyLoadError::DependencyNotDynamic,
                       source.identity);
    }

    Elf32LoadResult load;
    if (plan_result.plan.type == Elf32ImageType::Dynamic) {
        const auto placement =
            place_elf32_dynamic(memory, source.image, options.placement);
        if (!placement) {
            auto result =
                failure(Elf32DependencyLoadError::PlacementFailed,
                        source.identity);
            result.placement_error = placement.error;
            result.load_error = placement.load_error;
            return result;
        }

        Elf32LoadOptions load_options;
        load_options.dynamic_base = placement.dynamic_base;
        load = load_elf32(memory, source.image, load_options);
    } else {
        load = load_elf32(memory, source.image);
    }

    if (!load) {
        auto result =
            failure(Elf32DependencyLoadError::LoadFailed, source.identity);
        result.load_error = load.error;
        return result;
    }

    object.identity = std::move(source.identity);
    object.image = std::move(source.image);
    object.load = std::move(load);
    return {};
}

[[nodiscard]] Elf32DependencyLoadResult inspect_object(
    const memory::MappedGuestMemory& memory,
    Elf32LoadedDependencyObject& object,
    const Elf32DependencyLoadOptions& options) {
    if (!object.load.dynamic_segment.has_value()) {
        return {};
    }

    const auto dynamic =
        parse_elf32_dynamic(memory, *object.load.dynamic_segment);
    if (!dynamic) {
        auto result =
            failure(Elf32DependencyLoadError::DynamicParseFailed,
                    object.identity);
        result.dynamic_error = dynamic.error;
        return result;
    }
    object.dynamic_entries = dynamic.entries;

    const auto metadata =
        build_elf32_linker_metadata(memory, object.load.load_bias,
                                    object.dynamic_entries);
    if (!metadata) {
        auto result =
            failure(Elf32DependencyLoadError::LinkerMetadataFailed,
                    object.identity);
        result.metadata_error = metadata.error;
        return result;
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
        return result;
    }
    object.linker_strings = strings.strings;
    return {};
}

[[nodiscard]] std::size_t find_identity(
    const Elf32DependencyGraph& graph,
    const std::string& identity) {
    for (std::size_t index = 0; index < graph.objects.size(); ++index) {
        if (graph.objects[index].identity == identity) {
            return index;
        }
    }
    return graph.objects.size();
}

}  // namespace

Elf32DependencyLoadResult load_elf32_dependency_graph(
    memory::MappedGuestMemory& memory,
    Elf32DependencyLoadSource root,
    Elf32DependencyProvider& provider,
    const Elf32DependencyLoadOptions& options) {
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

    std::vector<Elf32LoadResult> successful_loads;
    successful_loads.reserve(options.max_objects);

    Elf32DependencyGraph graph;
    graph.objects.reserve(options.max_objects);

    Elf32LoadedDependencyObject root_object;
    auto root_load =
        load_object(memory, std::move(root), options, false, root_object);
    if (!root_load) {
        return root_load;
    }
    successful_loads.push_back(root_object.load);

    auto root_inspect = inspect_object(memory, root_object, options);
    if (!root_inspect) {
        return rollback_failure(memory, successful_loads,
                                std::move(root_inspect));
    }

    const std::uint64_t direct_occurrences =
        static_cast<std::uint64_t>(root_object.linker_strings.needed.size());
    if (direct_occurrences > options.max_dependency_occurrences ||
        root_object.linker_strings.needed.size() >
            std::numeric_limits<std::uint32_t>::max()) {
        auto result =
            failure(Elf32DependencyLoadError::TooManyDependencyOccurrences,
                    root_object.identity);
        return rollback_failure(memory, successful_loads, std::move(result));
    }

    graph.objects.push_back(std::move(root_object));
    if (graph.objects[0].linker_strings.needed.empty()) {
        Elf32DependencyLoadResult result;
        result.graph = std::move(graph);
        return result;
    }

    const std::uint64_t remaining_total =
        options.max_total_image_bytes - root_image_bytes;
    const auto resolved = resolve_elf32_dependencies(
        graph.objects[0].linker_strings,
        provider,
        Elf32DependencyResolveOptions{
            .max_dependencies = static_cast<std::uint32_t>(
                graph.objects[0].linker_strings.needed.size()),
            .max_image_bytes = options.max_image_bytes,
            .max_total_image_bytes = remaining_total,
        });
    if (!resolved) {
        auto result =
            failure(Elf32DependencyLoadError::DependencyResolveFailed,
                    graph.objects[0].identity);
        result.dependency_error = resolved.error;
        return rollback_failure(memory, successful_loads, std::move(result));
    }

    for (const auto& dependency : resolved.dependencies.ordered) {
        const std::size_t known_index =
            find_identity(graph, dependency.identity);
        if (known_index != graph.objects.size()) {
            if (graph.objects[known_index].image != dependency.image) {
                auto result =
                    failure(Elf32DependencyLoadError::IdentityImageMismatch,
                            dependency.identity);
                result.requested_name = dependency.requested_name;
                return rollback_failure(memory, successful_loads,
                                        std::move(result));
            }
            graph.objects[0].dependencies.push_back(Elf32DependencyEdge{
                .requested_name = dependency.requested_name,
                .target_object = known_index,
            });
            continue;
        }

        if (graph.objects.size() >= options.max_objects) {
            auto result =
                failure(Elf32DependencyLoadError::TooManyObjects,
                        dependency.identity);
            result.requested_name = dependency.requested_name;
            return rollback_failure(memory, successful_loads,
                                    std::move(result));
        }

        Elf32LoadedDependencyObject child;
        auto child_load = load_object(
            memory,
            Elf32DependencyLoadSource{
                .identity = dependency.identity,
                .image = dependency.image,
            },
            options, true, child);
        if (!child_load) {
            child_load.requested_name = dependency.requested_name;
            return rollback_failure(memory, successful_loads,
                                    std::move(child_load));
        }
        successful_loads.push_back(child.load);

        auto child_inspect = inspect_object(memory, child, options);
        if (!child_inspect) {
            child_inspect.requested_name = dependency.requested_name;
            return rollback_failure(memory, successful_loads,
                                    std::move(child_inspect));
        }
        if (!child.linker_strings.needed.empty()) {
            auto result =
                failure(Elf32DependencyLoadError::DependenciesNotImplemented,
                        child.identity);
            result.requested_name = child.linker_strings.needed.front();
            return rollback_failure(memory, successful_loads,
                                    std::move(result));
        }

        const std::size_t child_index = graph.objects.size();
        graph.objects.push_back(std::move(child));
        graph.objects[0].dependencies.push_back(Elf32DependencyEdge{
            .requested_name = dependency.requested_name,
            .target_object = child_index,
        });
    }

    Elf32DependencyLoadResult result;
    result.graph = std::move(graph);
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
