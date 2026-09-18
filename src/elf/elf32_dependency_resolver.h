#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "elf/elf32_linker_strings.h"

namespace liba32android::elf {

enum class Elf32DependencyProviderError : std::uint8_t {
    None = 0,
    NotFound,
    Failed,
};

struct Elf32DependencySource {
    std::string identity;
    std::vector<std::uint8_t> image;
};

struct Elf32DependencyProviderResult {
    Elf32DependencyProviderError error{Elf32DependencyProviderError::None};
    Elf32DependencySource source;

    [[nodiscard]] explicit operator bool() const noexcept {
        return error == Elf32DependencyProviderError::None;
    }
};

class Elf32DependencyProvider {
public:
    virtual ~Elf32DependencyProvider() = default;

    [[nodiscard]] virtual Elf32DependencyProviderResult resolve(
        std::string_view requested_name,
        std::uint64_t max_image_bytes) = 0;
};

struct Elf32DependencyResolveOptions {
    std::uint32_t max_dependencies{};
    std::uint64_t max_image_bytes{};
    std::uint64_t max_total_image_bytes{};
};

struct Elf32ResolvedDependency {
    std::string requested_name;
    std::string identity;
    std::vector<std::uint8_t> image;
};

struct Elf32ResolvedDependencies {
    std::vector<Elf32ResolvedDependency> ordered;
};

enum class Elf32DependencyResolveError : std::uint8_t {
    None = 0,
    TooManyDependencies,
    EmptyDependencyName,
    DependencyNotFound,
    ProviderFailed,
    EmptyProviderIdentity,
    EmptyDependencyImage,
    ImageTooLarge,
    TotalImageBytesExceeded,
};

struct Elf32DependencyResolveResult {
    Elf32DependencyResolveError error{Elf32DependencyResolveError::None};
    Elf32ResolvedDependencies dependencies;

    [[nodiscard]] explicit operator bool() const noexcept {
        return error == Elf32DependencyResolveError::None;
    }
};

// Resolve the ordered DT_NEEDED occurrences through a caller-owned provider.
// T001 establishes ordered acquisition and ownership only. Resource/error
// hardening beyond the dependency-count and empty-name checks is added by T002.
[[nodiscard]] Elf32DependencyResolveResult resolve_elf32_dependencies(
    const Elf32LinkerStrings& strings,
    Elf32DependencyProvider& provider,
    const Elf32DependencyResolveOptions& options);

[[nodiscard]] const char* to_string(Elf32DependencyResolveError error) noexcept;

}  // namespace liba32android::elf
