#include <cstdint>
#include <iostream>
#include <vector>

#include "elf/elf32_load_plan.h"
#include "memory/guest_memory.h"

namespace {

using liba32android::elf::Elf32ImageType;
using liba32android::elf::Elf32LoadError;
using liba32android::elf::plan_elf32_load;
using liba32android::memory::MappedGuestMemory;

constexpr std::size_t kHeaderSize = 52;
constexpr std::size_t kProgramHeaderSize = 32;
constexpr std::size_t kProgramHeaderOffset = kHeaderSize;

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
                                     std::uint32_t alignment) {
    std::vector<std::uint8_t> image(0x200, 0);
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
    write_u16(image, 44, 1);

    write_u32(image, kProgramHeaderOffset + 0, 1);
    write_u32(image, kProgramHeaderOffset + 4, 0);
    write_u32(image, kProgramHeaderOffset + 8, virtual_base);
    write_u32(image, kProgramHeaderOffset + 16, 0x100);
    write_u32(image, kProgramHeaderOffset + 20, 0x2100);
    write_u32(image, kProgramHeaderOffset + 24, 5);
    write_u32(image, kProgramHeaderOffset + 28, alignment);
    return image;
}

std::uint64_t align_down(std::uint64_t value, std::uint64_t alignment) {
    return value - (value % alignment);
}

std::uint64_t align_up(std::uint64_t value, std::uint64_t alignment) {
    const std::uint64_t remainder = value % alignment;
    return remainder == 0 ? value : value + (alignment - remainder);
}

int test_dynamic_layout() {
    MappedGuestMemory memory;
    auto image = make_image(3, 0, 0x4000);

    const auto result = plan_elf32_load(memory, image);
    if (!result || result.plan.type != Elf32ImageType::Dynamic) {
        return fail("ET_DYN load plan was not produced");
    }

    const std::uint64_t page = memory.page_size();
    if (result.plan.entry != 0x80 ||
        result.plan.minimum_page != 0 ||
        result.plan.maximum_page_end != align_up(0x2100, page) ||
        result.plan.required_load_bias_alignment != 0x4000 ||
        result.plan.segments.size() != 1) {
        return fail("ET_DYN load plan metadata mismatch");
    }

    if (memory.is_mapped(0)) {
        return fail("load planning unexpectedly mutated guest memory");
    }
    return 0;
}

int test_exec_layout() {
    MappedGuestMemory memory;
    constexpr std::uint32_t virtual_base = 0x12000;
    auto image = make_image(2, virtual_base, 0x1000);

    const auto result = plan_elf32_load(memory, image);
    if (!result || result.plan.type != Elf32ImageType::Executable) {
        return fail("ET_EXEC load plan was not produced");
    }

    const std::uint64_t page = memory.page_size();
    if (result.plan.entry != virtual_base + 0x80 ||
        result.plan.minimum_page != align_down(virtual_base, page) ||
        result.plan.maximum_page_end != align_up(virtual_base + 0x2100, page) ||
        result.plan.required_load_bias_alignment != 0x1000) {
        return fail("ET_EXEC load plan metadata mismatch");
    }
    return 0;
}

int test_validation_reuse() {
    MappedGuestMemory memory;
    auto image = make_image(3, 0, 0x3000);

    const auto result = plan_elf32_load(memory, image);
    if (result || result.error != Elf32LoadError::SegmentAlignmentInvalid) {
        return fail("load plan did not preserve ELF segment-alignment validation");
    }
    if (memory.is_mapped(0)) {
        return fail("failed load planning unexpectedly mutated guest memory");
    }
    return 0;
}

}  // namespace

int main() {
    if (test_dynamic_layout() != 0) return 1;
    if (test_exec_layout() != 0) return 1;
    if (test_validation_reuse() != 0) return 1;
    return 0;
}
