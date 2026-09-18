#include "elf/elf32_dependency_resolver.h"

#include <cstddef>
#include <utility>

namespace liba32android::elf {
namespace {

[[nodiscard]] Elf32DependencyResolveResult failure(Elf32DependencyResolveError error) {
    Elf32DependencyResolveResult result;
    result.error = error;
    return result;
}

}  // namespace

Elf32DependencyResolveResult resolve_elf32_dependencies(
    const Elf32LinkerStrings& strings,
    Elf32DependencyProvider& provider,
    const Elf32DependencyResolveOptions& options) {
    if (strings.needed.size() > options.max_dependencies) {
        return failure(Elf32DependencyResolveError::TooManyDependencies);
    }

    Elf32DependencyResolveResult result;
    result.dependencies.ordered.reserve(strings.needed.size());

    for (const std::string& requested_name : strings.needed) {
        if (requested_name.empty()) {
            return failure(Elf32DependencyResolveError::EmptyDependencyName);
        }

        Elf32DependencyProviderResult provider_result =
            provider.resolve(requested_name, options.max_image_bytes);
        if (!provider_result) {
            // T002 will distinguish provider NotFound from general provider
            // failure and add the remaining provider-result validation.
            return failure(Elf32DependencyResolveError::ProviderFailed);
        }

        result.dependencies.ordered.push_back(Elf32ResolvedDependency{
            .requested_name = requested_name,
            .identity = std::move(provider_result.source.identity),
            .image = std::move(provider_result.source.image),
        });
    }

    return result;
}

const char* to_string(Elf32DependencyResolveError error) noexcept {
    switch (error) {
    case Elf32DependencyResolveError::None:
        return "none";
    case Elf32DependencyResolveError::TooManyDependencies:
        return "too_many_dependencies";
    case Elf32DependencyResolveError::EmptyDependencyName:
        return "empty_dependency_name";
    case Elf32DependencyResolveError::DependencyNotFound:
        return "dependency_not_found";
    case Elf32DependencyResolveError::ProviderFailed:
        return "provider_failed";
    case Elf32DependencyResolveError::EmptyProviderIdentity:
        return "empty_provider_identity";
    case Elf32DependencyResolveError::EmptyDependencyImage:
        return "empty_dependency_image";
    case Elf32DependencyResolveError::ImageTooLarge:
        return "image_too_large";
    case Elf32DependencyResolveError::TotalImageBytesExceeded:
        return "total_image_bytes_exceeded";
    }
    return "unknown";
}

}  // namespace liba32android::elf
