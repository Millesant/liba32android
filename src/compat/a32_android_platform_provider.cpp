#include "compat/a32_android_platform_provider.h"

#include <cstdint>
#include <string_view>

namespace liba32android::compat {
namespace {

[[nodiscard]] elf::Elf32DependencyProviderResult failure(
    elf::Elf32DependencyProviderError error) {
    elf::Elf32DependencyProviderResult result;
    result.error = error;
    return result;
}

}  // namespace

elf::Elf32DependencyProviderResult A32AndroidPlatformProvider::resolve(
    std::string_view requested_name,
    std::uint64_t max_image_bytes) {
    return resolve_for({}, requested_name, max_image_bytes);
}

elf::Elf32DependencyProviderResult A32AndroidPlatformProvider::resolve_for(
    std::string_view requester_identity,
    std::string_view requested_name,
    std::uint64_t max_image_bytes) {
    // Feature 028 has one concrete platform-library slot. Unknown names remain
    // NotFound so an enclosing provider chain can continue normally.
    if (requested_name != kA32AndroidLogShimSoname) {
        return failure(elf::Elf32DependencyProviderError::NotFound);
    }

    switch (policy_.decide(requester_identity, requested_name)) {
    case A32AndroidPlatformAccessDecision::Allow:
        return catalog_.resolve(requested_name, max_image_bytes);
    case A32AndroidPlatformAccessDecision::NotFound:
        return failure(elf::Elf32DependencyProviderError::NotFound);
    case A32AndroidPlatformAccessDecision::Failed:
        return failure(elf::Elf32DependencyProviderError::Failed);
    }

    return failure(elf::Elf32DependencyProviderError::Failed);
}

}  // namespace liba32android::compat
