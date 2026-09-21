#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string_view>
#include <utility>
#include <vector>

#include "elf/elf32_dependency_loader.h"
#include "memory/guest_memory.h"

namespace {

using liba32android::elf::Elf32DependencyLoadError;
using liba32android::elf::Elf32DependencyLoadOptions;
using liba32android::elf::Elf32DependencyLoadSource;
using liba32android::elf::Elf32DependencyProvider;
using liba32android::elf::Elf32DependencyProviderError;
using liba32android::elf::Elf32DependencyProviderResult;
using liba32android::elf::Elf32DynamicError;
using liba32android::elf::load_elf32_dependency_graph;
using liba32android::memory::MappedGuestMemory;
using liba32android::memory::MemoryPermission;

constexpr std::size_t kHeaderSize = 52;
constexpr std::size_t kProgramHeaderSize = 32;
constexpr std::size_t kProgramHeaderOffset = kHeaderSize;
constexpr std::size_t kFirstProgramHeader = kProgramHeaderOffset;
constexpr std::size_t kSecondProgramHeader =
    kProgramHeaderOffset + kProgramHeaderSize;
constexpr std::size_t kThirdProgramHeader =
    kProgramHeaderOffset + 2 * kProgramHeaderSize;

int fail(const char* message) {
    std::cerr << message << '\n';
    return 1;
}

void write_u16(std::vector<std::uint8_t>& image,
               std::size_t offset,
               std::uint16_t value) {
    image[offset] = static_cast<std::uint8_t>(value & 0xffU);
    image[offset + 1] = static_cast<std::uint8_t>((value >> 8U) & 0xffU);
}

void write_u32(std::vector<std::uint8_t>& image,
               std::size_t offset,
               std::uint32_t value) {
    image[offset] = static_cast<std::uint8_t>(value & 0xffU);
    image[offset + 1] = static_cast<std::uint8_t>((value >> 8U) & 0xffU);
    image[offset + 2] = static_cast<std::uint8_t>((value >> 16U) & 0xffU);
    image[offset + 3] = static_cast<std::uint8_t>((value >> 24U) & 0xffU);
}

std::vector<std::uint8_t> make_image(std::uint16_t type,
                                     std::uint32_t virtual_base,
                                     bool with_dynamic,
                                     bool terminate_dynamic) {
    std::vector<std::uint8_t> image(0x1010, 0);
    image[0] = 0x7f;
    image[1] = 'E';
    image[2] = 'L';
    image[3] = 'F';
    image[4] = 1;
    image[5] = 1;
    image[6] = 1;

    write_u16(image, 16, type);
    write_u16(image, 18, 40);
    write_u32(image, 20, 1);
    write_u32(image, 24, virtual_base + 0x80);
    write_u32(image, 28, kProgramHeaderOffset);
    write_u16(image, 40, kHeaderSize);
    write_u16(image, 42, kProgramHeaderSize);
    write_u16(image, 44, with_dynamic ? 3 : 2);

    write_u32(image, kFirstProgramHeader + 0, 1);
    write_u32(image, kFirstProgramHeader + 4, 0);
    write_u32(image, kFirstProgramHeader + 8, virtual_base);
    write_u32(image, kFirstProgramHeader + 16, 0x100);
    write_u32(image, kFirstProgramHeader + 20, 0x100);
    write_u32(image, kFirstProgramHeader + 24, 5);
    write_u32(image, kFirstProgramHeader + 28, 0x4000);

    write_u32(image, kSecondProgramHeader + 0, 1);
    write_u32(image, kSecondProgramHeader + 4, 0x1000);
    write_u32(image, kSecondProgramHeader + 8, virtual_base + 0x4000);
    write_u32(image, kSecondProgramHeader + 16, 4);
    write_u32(image, kSecondProgramHeader + 20, 0x20);
    write_u32(image, kSecondProgramHeader + 24, 6);
    write_u32(image, kSecondProgramHeader + 28, 0x4000);

    image[0x1000] = 0x78;
    image[0x1001] = 0x56;
    image[0x1002] = 0x34;
    image[0x1003] = 0x12;

    if (with_dynamic) {
        write_u32(image, kThirdProgramHeader + 0, 2);
        write_u32(image, kThirdProgramHeader + 4, 0xc0);
        write_u32(image, kThirdProgramHeader + 8, virtual_base + 0xc0);
        write_u32(image, kThirdProgramHeader + 16, 8);
        write_u32(image, kThirdProgramHeader + 20, 8);
        write_u32(image, kThirdProgramHeader + 24, 4);
        write_u32(image, kThirdProgramHeader + 28, 4);

        if (!terminate_dynamic) {
            write_u32(image, 0xc0, 0x70000001U);
            write_u32(image, 0xc4, 0x12345678U);
        }
    }

    return image;
}

class FailIfCalledProvider final : public Elf32DependencyProvider {
public:
    std::size_t calls{};

    Elf32DependencyProviderResult resolve(
        std::string_view,
        std::uint64_t) override {
        ++calls;
        Elf32DependencyProviderResult result;
        result.error = Elf32DependencyProviderError::Failed;
        return result;
    }
};

Elf32DependencyLoadOptions options() {
    Elf32DependencyLoadOptions result;
    result.max_objects = 8;
    result.max_depth = 8;
    result.max_dependency_occurrences = 32;
    result.max_image_bytes = 1U << 20;
    result.max_total_image_bytes = 8U << 20;
    result.max_string_bytes = 4096;
    result.placement.search_begin = 0x10000;
    result.placement.search_end_exclusive = 0x80000;
    return result;
}

int test_exec_root_without_dynamic() {
    constexpr std::uint32_t fixed_base = 0x10000;
    MappedGuestMemory memory;
    FailIfCalledProvider provider;

    auto result = load_elf32_dependency_graph(
        memory,
        Elf32DependencyLoadSource{
            .identity = "root-exec",
            .image = make_image(2, fixed_base, false, true),
        },
        provider,
        options());

    if (!result || result.graph.objects.size() != 1) {
        return fail("dependency-free ET_EXEC root did not load");
    }
    const auto& root = result.graph.objects[0];
    if (root.identity != "root-exec" || root.load.load_bias != 0 ||
        root.load.entry != fixed_base + 0x80 ||
        !root.dynamic_entries.empty() ||
        !root.linker_strings.needed.empty() ||
        !root.dependencies.empty()) {
        return fail("ET_EXEC root graph metadata was incorrect");
    }
    if (!memory.is_mapped(fixed_base) ||
        !memory.is_mapped(fixed_base + 0x4000) ||
        provider.calls != 0) {
        return fail("ET_EXEC root mapping/provider behavior was incorrect");
    }
    return 0;
}

int test_dynamic_root_automatic_placement() {
    MappedGuestMemory memory;
    FailIfCalledProvider provider;

    auto result = load_elf32_dependency_graph(
        memory,
        Elf32DependencyLoadSource{
            .identity = "root-dyn",
            .image = make_image(3, 0, true, true),
        },
        provider,
        options());

    if (!result || result.graph.objects.size() != 1) {
        return fail("dependency-free ET_DYN root did not load");
    }
    const auto& root = result.graph.objects[0];
    if (root.load.load_bias != 0x10000 ||
        (root.load.load_bias % 0x4000) != 0 ||
        root.dynamic_entries.size() != 1 ||
        root.dynamic_entries[0].tag != 0 ||
        !root.linker_strings.needed.empty() ||
        provider.calls != 0) {
        return fail("ET_DYN root was not automatically placed/parsed as expected");
    }
    if (!memory.is_mapped(root.load.load_bias) ||
        !memory.is_mapped(root.load.load_bias + 0x4000)) {
        return fail("ET_DYN root mappings were not retained on success");
    }
    return 0;
}

int test_preflight_failures_do_not_mutate_memory() {
    MappedGuestMemory memory;
    FailIfCalledProvider provider;
    const auto rw = MemoryPermission::Read | MemoryPermission::Write;
    const std::uint32_t sentinel = 0x70000;
    if (!memory.map(sentinel, memory.page_size(), rw)) {
        return fail("could not create preexisting sentinel mapping");
    }

    {
        auto invalid = options();
        invalid.max_objects = 0;
        const auto result = load_elf32_dependency_graph(
            memory,
            Elf32DependencyLoadSource{
                .identity = "root",
                .image = make_image(3, 0, false, true),
            },
            provider,
            invalid);
        if (result.error != Elf32DependencyLoadError::InvalidOptions) {
            return fail("invalid graph options were not rejected");
        }
    }

    {
        auto limited = options();
        limited.max_image_bytes = 1;
        const auto result = load_elf32_dependency_graph(
            memory,
            Elf32DependencyLoadSource{
                .identity = "root",
                .image = make_image(3, 0, false, true),
            },
            provider,
            limited);
        if (result.error != Elf32DependencyLoadError::ImageTooLarge) {
            return fail("root per-image byte ceiling was not enforced");
        }
    }

    if (!memory.is_mapped(sentinel) || memory.permissions(sentinel) != rw ||
        memory.is_mapped(0x10000) || provider.calls != 0) {
        return fail("preflight failure mutated guest memory or called provider");
    }
    return 0;
}

int test_post_load_dynamic_failure_rolls_back_root_only() {
    MappedGuestMemory memory;
    FailIfCalledProvider provider;
    const auto rw = MemoryPermission::Read | MemoryPermission::Write;
    const std::uint32_t sentinel = 0x70000;
    if (!memory.map(sentinel, memory.page_size(), rw)) {
        return fail("could not create rollback sentinel mapping");
    }

    const auto result = load_elf32_dependency_graph(
        memory,
        Elf32DependencyLoadSource{
            .identity = "bad-dynamic-root",
            .image = make_image(3, 0, true, false),
        },
        provider,
        options());

    if (result.error != Elf32DependencyLoadError::DynamicParseFailed ||
        result.dynamic_error != Elf32DynamicError::Unterminated ||
        !result.graph.objects.empty()) {
        return fail("unterminated dynamic root did not fail transactionally");
    }
    if (memory.is_mapped(0x10000) || memory.is_mapped(0x14000)) {
        return fail("failed root mappings were not rolled back");
    }
    if (!memory.is_mapped(sentinel) || memory.permissions(sentinel) != rw) {
        return fail("rollback disturbed preexisting guest mapping");
    }
    if (provider.calls != 0) {
        return fail("provider was called during T001 rollback case");
    }
    return 0;
}

}  // namespace

int main() {
    if (const int status = test_exec_root_without_dynamic(); status != 0) return status;
    if (const int status = test_dynamic_root_automatic_placement(); status != 0) return status;
    if (const int status = test_preflight_failures_do_not_mutate_memory(); status != 0) return status;
    if (const int status = test_post_load_dynamic_failure_rolls_back_root_only();
        status != 0) {
        return status;
    }
    return 0;
}
