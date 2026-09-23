#include <array>
#include <cstdint>
#include <iostream>

#include "elf/elf32_relocation.h"
#include "memory/guest_memory.h"

namespace {

using liba32android::elf::Elf32DependencyGraph;
using liba32android::elf::Elf32RelTableMetadata;
using liba32android::elf::Elf32RelocationOptions;
using liba32android::elf::Elf32RelocationPlanError;
using liba32android::elf::build_elf32_rel_relocation_plan;
using liba32android::elf::kRArmAbs32;
using liba32android::elf::kRArmGlobDat;
using liba32android::elf::kRArmNone;
using liba32android::elf::kRArmRelative;
using liba32android::memory::LinearGuestMemory;

constexpr std::uint32_t kMemoryBase = 0x1000;
constexpr std::uint32_t kRelTable = 0x1100;

int fail(const char* message) {
    std::cerr << message << '\n';
    return 1;
}

bool write_u32(LinearGuestMemory& memory,
               std::uint32_t address,
               std::uint32_t value) {
    const std::array<std::uint8_t, 4> bytes{
        static_cast<std::uint8_t>(value),
        static_cast<std::uint8_t>(value >> 8U),
        static_cast<std::uint8_t>(value >> 16U),
        static_cast<std::uint8_t>(value >> 24U),
    };
    return memory.write(address, bytes);
}

bool read_u32(const LinearGuestMemory& memory,
              std::uint32_t address,
              std::uint32_t& value) {
    std::array<std::uint8_t, 4> bytes{};
    if (!memory.read(address, bytes)) return false;
    value = static_cast<std::uint32_t>(bytes[0]) |
            (static_cast<std::uint32_t>(bytes[1]) << 8U) |
            (static_cast<std::uint32_t>(bytes[2]) << 16U) |
            (static_cast<std::uint32_t>(bytes[3]) << 24U);
    return true;
}

bool write_rel(LinearGuestMemory& memory,
               std::uint32_t index,
               std::uint32_t offset,
               std::uint32_t symbol_index,
               std::uint8_t type) {
    return write_u32(memory, kRelTable + index * 8U, offset) &&
           write_u32(memory, kRelTable + index * 8U + 4U,
                     (symbol_index << 8U) | type);
}

Elf32DependencyGraph graph_with_rel(std::uint32_t load_bias,
                                    std::uint32_t table_address,
                                    std::uint32_t table_size) {
    Elf32DependencyGraph graph;
    graph.objects.resize(1);
    graph.objects[0].identity = "relocation-test";
    graph.objects[0].load.load_bias = load_bias;
    graph.objects[0].linker_metadata.rel_table =
        Elf32RelTableMetadata{
            .guest_address = table_address,
            .size = table_size,
            .entry_size = 8,
        };
    return graph;
}

Elf32RelocationOptions options(std::uint32_t max_relocations = 16) {
    Elf32RelocationOptions result;
    result.max_relocations = max_relocations;
    return result;
}

int test_empty_and_exact_decode() {
    {
        LinearGuestMemory memory(0x4000, kMemoryBase);
        Elf32DependencyGraph graph;
        graph.objects.resize(1);
        const auto result =
            build_elf32_rel_relocation_plan(memory, graph, 0, options(0));
        if (!result || !result.plan.entries.empty()) {
            return fail("object without REL table did not produce empty success");
        }
    }

    LinearGuestMemory memory(0x5000, kMemoryBase);
    auto graph = graph_with_rel(0x1000, kRelTable, 32);

    if (!write_rel(memory, 0, 0x7000, 9, kRArmNone) ||
        !write_rel(memory, 1, 0x1000, 3, kRArmAbs32) ||
        !write_rel(memory, 2, 0x1004, 2, kRArmGlobDat) ||
        !write_rel(memory, 3, 0x1008, 0, kRArmRelative) ||
        !write_u32(memory, 0x2000, 0x11223344U) ||
        !write_u32(memory, 0x2004, 0xaabbccddU) ||
        !write_u32(memory, 0x2008, 0x01020304U)) {
        return fail("could not stage valid REL plan");
    }

    const auto result =
        build_elf32_rel_relocation_plan(memory, graph, 0, options());
    if (!result || result.plan.object_index != 0 ||
        result.plan.entries.size() != 4) {
        return fail("valid REL table did not decode");
    }

    const auto& none = result.plan.entries[0];
    if (none.offset != 0x7000 || none.symbol_index != 9 ||
        none.type != kRArmNone ||
        none.place_guest_address != 0x8000 ||
        none.original_word.has_value()) {
        return fail("R_ARM_NONE plan metadata was incorrect or read its target");
    }

    const auto& abs = result.plan.entries[1];
    const auto& glob = result.plan.entries[2];
    const auto& relative = result.plan.entries[3];
    if (abs.place_guest_address != 0x2000 ||
        abs.symbol_index != 3 ||
        !abs.original_word.has_value() ||
        *abs.original_word != 0x11223344U ||
        glob.place_guest_address != 0x2004 ||
        glob.symbol_index != 2 ||
        !glob.original_word.has_value() ||
        *glob.original_word != 0xaabbccddU ||
        relative.place_guest_address != 0x2008 ||
        relative.symbol_index != 0 ||
        !relative.original_word.has_value() ||
        *relative.original_word != 0x01020304U) {
        return fail("supported REL entry metadata/addends were decoded incorrectly");
    }

    std::uint32_t value = 0;
    if (!read_u32(memory, 0x2000, value) || value != 0x11223344U ||
        !read_u32(memory, 0x2004, value) || value != 0xaabbccddU ||
        !read_u32(memory, 0x2008, value) || value != 0x01020304U) {
        return fail("read-only REL planning mutated guest target words");
    }

    return 0;
}

int test_limits_and_graph_inputs() {
    LinearGuestMemory memory(0x4000, kMemoryBase);
    auto graph = graph_with_rel(0x1000, kRelTable, 16);
    if (!write_rel(memory, 0, 0x1000, 1, kRArmAbs32) ||
        !write_rel(memory, 1, 0x1004, 2, kRArmGlobDat) ||
        !write_u32(memory, 0x2000, 0) ||
        !write_u32(memory, 0x2004, 0)) {
        return fail("could not stage limit test");
    }

    if (build_elf32_rel_relocation_plan(memory, graph, 0, options(0)).error !=
        Elf32RelocationPlanError::InvalidOptions) {
        return fail("zero relocation limit with REL table was not rejected");
    }
    if (build_elf32_rel_relocation_plan(memory, graph, 0, options(1)).error !=
        Elf32RelocationPlanError::TooManyRelocations) {
        return fail("relocation count ceiling was not enforced");
    }
    if (build_elf32_rel_relocation_plan(memory, graph, 1, options()).error !=
        Elf32RelocationPlanError::InvalidGraphObject) {
        return fail("invalid graph object index was not rejected");
    }

    graph.objects[0].linker_metadata.rel_table->entry_size = 4;
    if (build_elf32_rel_relocation_plan(memory, graph, 0, options()).error !=
        Elf32RelocationPlanError::RelocationReadFailed) {
        return fail("crafted invalid REL entry size was not rejected defensively");
    }
    return 0;
}

int test_read_place_and_type_failures() {
    {
        LinearGuestMemory memory(0x1000, kMemoryBase);
        auto graph = graph_with_rel(0x1000, 0x1ffc, 8);
        if (build_elf32_rel_relocation_plan(memory, graph, 0, options()).error !=
            Elf32RelocationPlanError::RelocationReadFailed) {
            return fail("unreadable REL entry was not rejected");
        }
    }

    {
        LinearGuestMemory memory(0x4000, kMemoryBase);
        auto graph = graph_with_rel(0xfffffff0U, kRelTable, 8);
        if (!write_rel(memory, 0, 0x20, 1, kRArmAbs32)) {
            return fail("could not stage place-overflow test");
        }
        if (build_elf32_rel_relocation_plan(memory, graph, 0, options()).error !=
            Elf32RelocationPlanError::PlaceOverflow) {
            return fail("REL place overflow was not rejected");
        }
    }

    {
        LinearGuestMemory memory(0x4000, kMemoryBase);
        auto graph = graph_with_rel(0x1000, kRelTable, 8);
        if (!write_rel(memory, 0, 0x1001, 1, kRArmAbs32)) {
            return fail("could not stage alignment test");
        }
        if (build_elf32_rel_relocation_plan(memory, graph, 0, options()).error !=
            Elf32RelocationPlanError::UnalignedPlace) {
            return fail("unaligned supported relocation place was not rejected");
        }
    }

    {
        LinearGuestMemory memory(0x2000, kMemoryBase);
        auto graph = graph_with_rel(0x1000, kRelTable, 8);
        if (!write_rel(memory, 0, 0x3000, 1, kRArmGlobDat)) {
            return fail("could not stage target-read test");
        }
        if (build_elf32_rel_relocation_plan(memory, graph, 0, options()).error !=
            Elf32RelocationPlanError::TargetReadFailed) {
            return fail("unreadable supported relocation target was not rejected");
        }
    }

    {
        LinearGuestMemory memory(0x2000, kMemoryBase);
        auto graph = graph_with_rel(0x1000, kRelTable, 8);
        if (!write_rel(memory, 0, 0x3000, 1, 22)) {
            return fail("could not stage unsupported-type test");
        }
        if (build_elf32_rel_relocation_plan(memory, graph, 0, options()).error !=
            Elf32RelocationPlanError::UnsupportedRelocationType) {
            return fail("unsupported relocation type was not rejected before target access");
        }
    }

    return 0;
}

int test_duplicate_target_rejected_without_mutation() {
    LinearGuestMemory memory(0x4000, kMemoryBase);
    auto graph = graph_with_rel(0x1000, kRelTable, 16);
    if (!write_rel(memory, 0, 0x1000, 1, kRArmAbs32) ||
        !write_rel(memory, 1, 0x1000, 2, kRArmGlobDat) ||
        !write_u32(memory, 0x2000, 0xdeadbeefU)) {
        return fail("could not stage duplicate-target test");
    }

    const auto result =
        build_elf32_rel_relocation_plan(memory, graph, 0, options());
    if (result.error != Elf32RelocationPlanError::DuplicateTarget) {
        return fail("duplicate writable relocation target was not rejected");
    }

    std::uint32_t value = 0;
    if (!read_u32(memory, 0x2000, value) || value != 0xdeadbeefU) {
        return fail("duplicate-target planning failure mutated guest memory");
    }
    return 0;
}

}  // namespace

int main() {
    if (const int status = test_empty_and_exact_decode(); status != 0) return status;
    if (const int status = test_limits_and_graph_inputs(); status != 0) return status;
    if (const int status = test_read_place_and_type_failures(); status != 0) return status;
    if (const int status = test_duplicate_target_rejected_without_mutation(); status != 0) return status;
    return 0;
}
