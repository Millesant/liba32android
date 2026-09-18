#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "elf/elf32_dependency_resolver.h"

namespace {

using liba32android::elf::Elf32DependencyProvider;
using liba32android::elf::Elf32DependencyProviderError;
using liba32android::elf::Elf32DependencyProviderResult;
using liba32android::elf::Elf32DependencyResolveError;
using liba32android::elf::Elf32DependencyResolveOptions;
using liba32android::elf::Elf32DependencySource;
using liba32android::elf::Elf32LinkerStrings;
using liba32android::elf::resolve_elf32_dependencies;

int fail(const char* message) {
    std::cerr << message << '\n';
    return 1;
}

Elf32DependencyProviderResult success(std::string identity,
                                      std::vector<std::uint8_t> image) {
    Elf32DependencyProviderResult result;
    result.source.identity = std::move(identity);
    result.source.image = std::move(image);
    return result;
}

class RecordingProvider final : public Elf32DependencyProvider {
public:
    std::vector<Elf32DependencyProviderResult> responses;
    std::vector<std::string> requests;
    std::vector<std::uint64_t> limits;

    Elf32DependencyProviderResult resolve(
        std::string_view requested_name,
        std::uint64_t max_image_bytes) override {
        requests.emplace_back(requested_name.data(), requested_name.size());
        limits.push_back(max_image_bytes);

        const std::size_t index = requests.size() - 1;
        if (index >= responses.size()) {
            Elf32DependencyProviderResult result;
            result.error = Elf32DependencyProviderError::Failed;
            return result;
        }
        return responses[index];
    }
};

Elf32DependencyResolveOptions options(std::uint32_t max_dependencies = 8) {
    return Elf32DependencyResolveOptions{
        .max_dependencies = max_dependencies,
        .max_image_bytes = 4096,
        .max_total_image_bytes = 16384,
    };
}

int test_ordered_success_and_owned_results() {
    Elf32LinkerStrings strings;
    strings.needed = {"liba.so", "libb.so"};

    RecordingProvider provider;
    provider.responses = {
        success("provider:a", {0x7f, 'E', 'L', 'F', 1}),
        success("provider:b", {1, 2, 3, 4}),
    };

    auto result = resolve_elf32_dependencies(strings, provider, options());
    if (!result) return fail("ordered dependency resolution failed");
    if (provider.requests != std::vector<std::string>{"liba.so", "libb.so"}) {
        return fail("provider request order did not match DT_NEEDED order");
    }
    if (provider.limits != std::vector<std::uint64_t>{4096, 4096}) {
        return fail("provider did not receive the configured image ceiling");
    }
    if (result.dependencies.ordered.size() != 2 ||
        result.dependencies.ordered[0].requested_name != "liba.so" ||
        result.dependencies.ordered[0].identity != "provider:a" ||
        result.dependencies.ordered[0].image !=
            std::vector<std::uint8_t>{0x7f, 'E', 'L', 'F', 1} ||
        result.dependencies.ordered[1].requested_name != "libb.so" ||
        result.dependencies.ordered[1].identity != "provider:b" ||
        result.dependencies.ordered[1].image !=
            std::vector<std::uint8_t>{1, 2, 3, 4}) {
        return fail("resolved dependencies did not preserve ordered owned results");
    }

    provider.responses.clear();
    if (result.dependencies.ordered[0].identity != "provider:a" ||
        result.dependencies.ordered[0].image.size() != 5) {
        return fail("resolved dependency result depended on provider storage lifetime");
    }
    return 0;
}

int test_repeated_occurrences_are_not_deduplicated() {
    Elf32LinkerStrings strings;
    strings.needed = {"libsame.so", "libsame.so"};

    RecordingProvider provider;
    provider.responses = {
        success("first", {1}),
        success("second", {2}),
    };

    const auto result = resolve_elf32_dependencies(strings, provider, options());
    if (!result) return fail("repeated dependency resolution failed");
    if (provider.requests != std::vector<std::string>{"libsame.so", "libsame.so"}) {
        return fail("repeated dependency did not cause one provider request per occurrence");
    }
    if (result.dependencies.ordered.size() != 2 ||
        result.dependencies.ordered[0].identity != "first" ||
        result.dependencies.ordered[1].identity != "second") {
        return fail("repeated dependency occurrences were deduplicated or reordered");
    }
    return 0;
}

int test_empty_set_and_count_precheck() {
    {
        Elf32LinkerStrings strings;
        RecordingProvider provider;
        const auto result = resolve_elf32_dependencies(strings, provider, options(0));
        if (!result || !result.dependencies.ordered.empty() || !provider.requests.empty()) {
            return fail("empty dependency set did not succeed without provider calls");
        }
    }

    {
        Elf32LinkerStrings strings;
        strings.needed = {"a.so", "b.so"};
        RecordingProvider provider;
        const auto result = resolve_elf32_dependencies(strings, provider, options(1));
        if (result.error != Elf32DependencyResolveError::TooManyDependencies ||
            !provider.requests.empty() || !result.dependencies.ordered.empty()) {
            return fail("dependency-count precheck did not fail before provider calls");
        }
    }
    return 0;
}

int test_empty_name_rejected_before_its_provider_call() {
    Elf32LinkerStrings strings;
    strings.needed = {"ok.so", ""};

    RecordingProvider provider;
    provider.responses = {
        success("ok", {1, 2, 3}),
    };

    const auto result = resolve_elf32_dependencies(strings, provider, options());
    if (result.error != Elf32DependencyResolveError::EmptyDependencyName) {
        return fail("empty dependency name was not rejected");
    }
    if (provider.requests != std::vector<std::string>{"ok.so"}) {
        return fail("provider was invoked for an empty dependency name");
    }
    if (!result.dependencies.ordered.empty()) {
        return fail("empty-name failure exposed a successful partial aggregate");
    }
    return 0;
}

int test_raw_bytes_and_slashes_forwarded_exactly() {
    Elf32LinkerStrings strings;
    const std::string raw_name{
        static_cast<char>(0xff),
        static_cast<char>(0x80),
        '/',
        'x',
    };
    strings.needed = {raw_name, "dir/sub/libx.so"};

    RecordingProvider provider;
    provider.responses = {
        success("raw", {9}),
        success("slash", {8}),
    };

    const auto result = resolve_elf32_dependencies(strings, provider, options());
    if (!result) return fail("raw dependency-name forwarding failed");
    if (provider.requests.size() != 2 ||
        provider.requests[0].size() != raw_name.size() ||
        provider.requests[0] != raw_name ||
        provider.requests[1] != "dir/sub/libx.so") {
        return fail("dependency request bytes were normalized or rewritten");
    }
    if (result.dependencies.ordered[0].requested_name != raw_name ||
        result.dependencies.ordered[1].requested_name != "dir/sub/libx.so") {
        return fail("resolved result did not retain exact requested-name bytes");
    }
    return 0;
}

}  // namespace

int main() {
    if (const int status = test_ordered_success_and_owned_results(); status != 0) return status;
    if (const int status = test_repeated_occurrences_are_not_deduplicated(); status != 0) return status;
    if (const int status = test_empty_set_and_count_precheck(); status != 0) return status;
    if (const int status = test_empty_name_rejected_before_its_provider_call(); status != 0) return status;
    if (const int status = test_raw_bytes_and_slashes_forwarded_exactly(); status != 0) return status;
    return 0;
}
