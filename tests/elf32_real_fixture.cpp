#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <span>
#include <string>
#include <vector>

#include "elf/elf32_loader.h"
#include "memory/guest_memory.h"

namespace {

using liba32android::elf::Elf32LoadOptions;
using liba32android::elf::Elf32LoadedSegment;
using liba32android::elf::load_elf32;
using liba32android::elf::to_string;
using liba32android::memory::MappedGuestMemory;
using liba32android::memory::MemoryPermission;
using liba32android::memory::has_permission;

constexpr std::size_t kElf32HeaderSize = 52;
constexpr std::size_t kElf32ProgramHeaderSize = 32;
constexpr std::uint16_t kElfTypeDyn = 3;
constexpr std::uint16_t kElfMachineArm = 40;
constexpr std::uint32_t kProgramTypeLoad = 1;
constexpr std::uint32_t kProgramTypeDynamic = 2;
constexpr std::uint32_t kFlagExecute = 1U << 0;
constexpr std::uint32_t kFlagWrite = 1U << 1;
constexpr std::uint32_t kFlagRead = 1U << 2;
constexpr std::uint64_t kFixtureLoadBias = 0x02000000ULL;
constexpr std::uint32_t kExpectedFixtureAlignment = 0x4000U;

struct LoadSegment {
    std::uint32_t offset{};
    std::uint32_t virtual_address{};
    std::uint32_t file_size{};
    std::uint32_t memory_size{};
    std::uint32_t flags{};
    std::uint32_t alignment{};
};

int fail(const std::string& message) {
    std::cerr << message << '\n';
    return 1;
}

std::uint16_t read_u16(std::span<const std::uint8_t> bytes, std::size_t offset) {
    return static_cast<std::uint16_t>(bytes[offset]) |
           static_cast<std::uint16_t>(bytes[offset + 1]) << 8U;
}

std::uint32_t read_u32(std::span<const std::uint8_t> bytes, std::size_t offset) {
    return static_cast<std::uint32_t>(bytes[offset]) |
           static_cast<std::uint32_t>(bytes[offset + 1]) << 8U |
           static_cast<std::uint32_t>(bytes[offset + 2]) << 16U |
           static_cast<std::uint32_t>(bytes[offset + 3]) << 24U;
}

std::uint64_t align_down(std::uint64_t value, std::uint64_t alignment) {
    return value - (value % alignment);
}

std::uint64_t align_up(std::uint64_t value, std::uint64_t alignment) {
    const std::uint64_t remainder = value % alignment;
    return remainder == 0 ? value : value + (alignment - remainder);
}

MemoryPermission permissions_from_flags(std::uint32_t flags) {
    MemoryPermission permissions = MemoryPermission::None;
    if ((flags & kFlagRead) != 0) permissions = permissions | MemoryPermission::Read;
    if ((flags & kFlagWrite) != 0) permissions = permissions | MemoryPermission::Write;
    if ((flags & kFlagExecute) != 0) permissions = permissions | MemoryPermission::Execute;
    return permissions;
}

std::vector<std::uint8_t> read_file(const char* path) {
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input) return {};

    const std::streamoff end = input.tellg();
    if (end <= 0 || static_cast<std::uint64_t>(end) > std::numeric_limits<std::size_t>::max()) {
        return {};
    }

    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(end));
    input.seekg(0, std::ios::beg);
    if (!input.read(reinterpret_cast<char*>(bytes.data()), end)) return {};
    return bytes;
}

const Elf32LoadedSegment* find_loaded_segment(const std::vector<Elf32LoadedSegment>& segments,
                                              std::uint32_t guest_address) {
    const auto it = std::find_if(segments.begin(), segments.end(), [&](const Elf32LoadedSegment& segment) {
        return segment.guest_address == guest_address;
    });
    return it == segments.end() ? nullptr : &*it;
}

bool all_zero(std::span<const std::uint8_t> bytes) {
    return std::all_of(bytes.begin(), bytes.end(), [](std::uint8_t byte) { return byte == 0; });
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        return fail("expected path to generated ARM32 fixture");
    }

    const std::vector<std::uint8_t> image = read_file(argv[1]);
    if (image.size() < kElf32HeaderSize) {
        return fail("generated fixture is missing or too small to be ELF32");
    }
    if (image[0] != 0x7f || image[1] != 'E' || image[2] != 'L' || image[3] != 'F' ||
        image[4] != 1 || image[5] != 1 || image[6] != 1) {
        return fail("generated fixture does not have the expected ELF32 little-endian identity");
    }
    if (read_u16(image, 16) != kElfTypeDyn || read_u16(image, 18) != kElfMachineArm) {
        return fail("generated fixture is not an ARM ELF32 ET_DYN image");
    }

    const std::uint32_t program_header_offset = read_u32(image, 28);
    const std::uint16_t program_header_size = read_u16(image, 42);
    const std::uint16_t program_header_count = read_u16(image, 44);
    if (program_header_size != kElf32ProgramHeaderSize ||
        static_cast<std::uint64_t>(program_header_offset) +
                static_cast<std::uint64_t>(program_header_size) * program_header_count >
            image.size()) {
        return fail("generated fixture has an invalid program-header table");
    }

    std::vector<LoadSegment> loads;
    bool has_dynamic = false;
    bool has_executable = false;
    bool has_writable = false;
    bool has_bss = false;
    std::uint32_t maximum_alignment = 0;

    for (std::uint16_t index = 0; index < program_header_count; ++index) {
        const std::size_t offset = static_cast<std::size_t>(program_header_offset) +
                                   static_cast<std::size_t>(index) * kElf32ProgramHeaderSize;
        const std::uint32_t type = read_u32(image, offset);
        if (type == kProgramTypeDynamic) {
            has_dynamic = true;
        }
        if (type != kProgramTypeLoad) {
            continue;
        }

        LoadSegment segment;
        segment.offset = read_u32(image, offset + 4);
        segment.virtual_address = read_u32(image, offset + 8);
        segment.file_size = read_u32(image, offset + 16);
        segment.memory_size = read_u32(image, offset + 20);
        segment.flags = read_u32(image, offset + 24);
        segment.alignment = read_u32(image, offset + 28);
        loads.push_back(segment);

        has_executable = has_executable || (segment.flags & kFlagExecute) != 0;
        has_writable = has_writable || (segment.flags & kFlagWrite) != 0;
        has_bss = has_bss || segment.memory_size > segment.file_size;
        maximum_alignment = std::max(maximum_alignment, segment.alignment);
    }

    if (loads.size() < 2 || !has_dynamic || !has_executable || !has_writable || !has_bss) {
        return fail("generated fixture does not contain the intended text/data/BSS dynamic-library layout");
    }
    if (maximum_alignment < kExpectedFixtureAlignment) {
        return fail("generated fixture PT_LOAD alignment is smaller than the requested 16 KiB");
    }

    MappedGuestMemory memory;
    const std::uint64_t host_page_size = memory.page_size();
    std::uint64_t minimum_page = std::numeric_limits<std::uint64_t>::max();
    for (const LoadSegment& segment : loads) {
        if (segment.memory_size == 0) continue;
        minimum_page = std::min(minimum_page, align_down(segment.virtual_address, host_page_size));
    }
    if (minimum_page == std::numeric_limits<std::uint64_t>::max()) {
        return fail("generated fixture has no non-empty PT_LOAD segment");
    }

    // Keep the load bias aligned to the strongest PT_LOAD alignment even when
    // this host happens to use 4 KiB pages. This preserves ELF congruence and
    // makes the same fixture suitable for future 16 KiB host-page validation.
    if ((kFixtureLoadBias % maximum_alignment) != 0) {
        return fail("test load bias is not aligned to the fixture PT_LOAD alignment");
    }
    const std::uint64_t dynamic_base_u64 = minimum_page + kFixtureLoadBias;
    if (dynamic_base_u64 > std::numeric_limits<std::uint32_t>::max() ||
        (dynamic_base_u64 % host_page_size) != 0) {
        return fail("fixture dynamic base is outside the 32-bit guest range or host-page unaligned");
    }

    Elf32LoadOptions options;
    options.dynamic_base = static_cast<std::uint32_t>(dynamic_base_u64);
    const auto result = load_elf32(memory, image, options);
    if (!result) {
        return fail(std::string("real ARM32 fixture load failed: ") + to_string(result.error));
    }
    if (result.load_bias != kFixtureLoadBias || result.segments.size() != loads.size()) {
        return fail("real ARM32 fixture returned unexpected load-bias/segment metadata");
    }

    for (std::size_t index = 0; index < loads.size(); ++index) {
        const LoadSegment& raw = loads[index];
        if (raw.memory_size == 0) continue;

        const std::uint64_t guest_u64 = static_cast<std::uint64_t>(raw.virtual_address) + result.load_bias;
        if (guest_u64 > std::numeric_limits<std::uint32_t>::max()) {
            return fail("fixture segment guest address overflowed after load bias");
        }
        const std::uint32_t guest = static_cast<std::uint32_t>(guest_u64);
        const Elf32LoadedSegment* loaded = find_loaded_segment(result.segments, guest);
        if (loaded == nullptr) {
            return fail("loader result omitted a real fixture PT_LOAD segment");
        }

        const MemoryPermission expected_permissions = permissions_from_flags(raw.flags);
        const std::uint64_t expected_mapping_start = align_down(guest_u64, host_page_size);
        const std::uint64_t expected_mapping_end = align_up(guest_u64 + raw.memory_size, host_page_size);
        if (loaded->file_size != raw.file_size || loaded->memory_size != raw.memory_size ||
            loaded->mapping_start != expected_mapping_start ||
            loaded->mapping_size != expected_mapping_end - expected_mapping_start ||
            loaded->permissions != expected_permissions) {
            return fail("loader metadata does not match a real fixture PT_LOAD header");
        }

        for (std::uint64_t page = expected_mapping_start; page < expected_mapping_end;
             page += host_page_size) {
            if (memory.permissions(static_cast<std::uint32_t>(page)) != expected_permissions) {
                return fail("final guest page permissions do not match the real fixture PT_LOAD flags");
            }
        }

        if (!has_permission(expected_permissions, MemoryPermission::Read)) {
            return fail("fixture unexpectedly contains a non-readable PT_LOAD segment");
        }

        if (raw.file_size != 0) {
            if (static_cast<std::uint64_t>(raw.offset) + raw.file_size > image.size()) {
                return fail("fixture PT_LOAD file bytes extend beyond the generated image");
            }
            std::vector<std::uint8_t> actual(raw.file_size);
            if (!memory.read(guest, actual)) {
                return fail("could not read loaded file bytes from real fixture guest memory");
            }
            const auto expected = std::span<const std::uint8_t>(image).subspan(raw.offset, raw.file_size);
            if (!std::equal(actual.begin(), actual.end(), expected.begin(), expected.end())) {
                return fail("real fixture file bytes differ after PT_LOAD mapping");
            }
        }

        if (raw.memory_size > raw.file_size) {
            std::vector<std::uint8_t> bss(raw.memory_size - raw.file_size, 0xff);
            if (!memory.read(guest + raw.file_size, bss) || !all_zero(bss)) {
                return fail("real fixture BSS tail was not zero-filled");
            }
        }

        std::cout << "fixture.pt_load." << index << ".offset=0x" << std::hex << raw.offset << '\n'
                  << "fixture.pt_load." << index << ".vaddr=0x" << raw.virtual_address << '\n'
                  << "fixture.pt_load." << index << ".filesz=0x" << raw.file_size << '\n'
                  << "fixture.pt_load." << index << ".memsz=0x" << raw.memory_size << '\n'
                  << "fixture.pt_load." << index << ".flags=0x" << raw.flags << '\n'
                  << "fixture.pt_load." << index << ".align=0x" << raw.alignment << '\n'
                  << std::dec;
    }

    std::cout << "fixture.kind=android_armv7_et_dyn\n"
              << "fixture.size=" << image.size() << '\n'
              << "fixture.pt_load.count=" << loads.size() << '\n'
              << "fixture.pt_dynamic=true\n"
              << "fixture.has_bss=true\n"
              << "fixture.max_p_align=" << maximum_alignment << '\n'
              << "fixture.host_page_size=" << host_page_size << '\n'
              << "fixture.load_bias=0x" << std::hex << result.load_bias << std::dec << '\n'
              << "fixture.status=PASS\n";
    return 0;
}
