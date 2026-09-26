#include <array>
#include <cstdint>
#include <iostream>
#include <vector>

#include "elf/elf32_lifecycle.h"
#include "memory/guest_memory.h"

namespace {

using liba32android::elf::Elf32DependencyEdge;
using liba32android::elf::Elf32DependencyGraph;
using liba32android::elf::Elf32FunctionArrayDecodeError;
using liba32android::elf::Elf32FunctionArrayDecodeOptions;
using liba32android::elf::Elf32FunctionArrayMetadata;
using liba32android::elf::Elf32InitPlanError;
using liba32android::elf::Elf32InitPlanOptions;
using liba32android::elf::decode_elf32_function_array;
using liba32android::elf::plan_elf32_init_array_calls;
using liba32android::memory::LinearGuestMemory;

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

int test_exact_decode_and_raw_sentinels() {
    LinearGuestMemory memory(0x100, 0x1000);
    constexpr std::uint32_t address = 0x1010;
    if (!write_u32(memory, address, 0U) ||
        !write_u32(memory, address + 4U, 0xffffffffU) ||
        !write_u32(memory, address + 8U, 0x12345679U)) {
        return fail("could not stage lifecycle array");
    }

    std::array<std::uint8_t, 12> before{};
    if (!memory.read(address, before)) {
        return fail("could not snapshot lifecycle array");
    }

    const auto result = decode_elf32_function_array(
        memory,
        Elf32FunctionArrayMetadata{.guest_address = address, .size = 12},
        Elf32FunctionArrayDecodeOptions{.max_entries = 3});
    if (!result ||
        result.entries !=
            std::vector<std::uint32_t>{0U, 0xffffffffU, 0x12345679U}) {
        return fail("lifecycle array decode changed order or sentinel values");
    }

    std::array<std::uint8_t, 12> after{};
    if (!memory.read(address, after) || after != before) {
        return fail("lifecycle array decode mutated guest memory");
    }
    return 0;
}

int test_entry_ceiling_preflights_before_read() {
    LinearGuestMemory memory(0x100, 0x1000);
    const auto result = decode_elf32_function_array(
        memory,
        Elf32FunctionArrayMetadata{
            .guest_address = 0x90000000U,
            .size = 12,
        },
        Elf32FunctionArrayDecodeOptions{.max_entries = 2});
    if (result.error != Elf32FunctionArrayDecodeError::TooManyEntries ||
        !result.entries.empty()) {
        return fail("lifecycle entry ceiling did not fail before reading");
    }
    return 0;
}

int test_invalid_size_and_range() {
    LinearGuestMemory memory(0x100, 0x1000);

    const auto bad_size = decode_elf32_function_array(
        memory,
        Elf32FunctionArrayMetadata{.guest_address = 0x1000, .size = 6},
        Elf32FunctionArrayDecodeOptions{.max_entries = 2});
    if (bad_size.error != Elf32FunctionArrayDecodeError::InvalidArraySize) {
        return fail("non-integral lifecycle array size was not rejected");
    }

    const auto overflow = decode_elf32_function_array(
        memory,
        Elf32FunctionArrayMetadata{
            .guest_address = 0xfffffffcU,
            .size = 8,
        },
        Elf32FunctionArrayDecodeOptions{.max_entries = 2});
    if (overflow.error != Elf32FunctionArrayDecodeError::RangeOverflow) {
        return fail("lifecycle decoder range overflow was not rejected");
    }
    return 0;
}

int test_unreadable_and_empty_arrays() {
    LinearGuestMemory memory(0x100, 0x1000);

    const auto unreadable = decode_elf32_function_array(
        memory,
        Elf32FunctionArrayMetadata{.guest_address = 0x2000, .size = 4},
        Elf32FunctionArrayDecodeOptions{.max_entries = 1});
    if (unreadable.error != Elf32FunctionArrayDecodeError::ReadFailed ||
        !unreadable.entries.empty()) {
        return fail("unreadable lifecycle array was not rejected cleanly");
    }

    const auto empty = decode_elf32_function_array(
        memory,
        Elf32FunctionArrayMetadata{
            .guest_address = 0xffffffffU,
            .size = 0,
        },
        Elf32FunctionArrayDecodeOptions{.max_entries = 0});
    if (!empty || !empty.entries.empty()) {
        return fail("zero-length lifecycle array did not decode as empty");
    }
    return 0;
}

bool stage_init_array(LinearGuestMemory& memory,
                      Elf32DependencyGraph& graph,
                      std::size_t object_index,
                      std::uint32_t address,
                      const std::vector<std::uint32_t>& entries) {
    if (object_index >= graph.objects.size()) return false;
    for (std::size_t index = 0; index < entries.size(); ++index) {
        if (!write_u32(memory,
                       address + static_cast<std::uint32_t>(index * 4U),
                       entries[index])) {
            return false;
        }
    }
    graph.objects[object_index].linker_metadata.init_array =
        Elf32FunctionArrayMetadata{
            .guest_address = address,
            .size = static_cast<std::uint32_t>(entries.size() * 4U),
        };
    return true;
}

int test_init_plan_dependency_order_cycles_and_sentinels() {
    LinearGuestMemory memory(0x200, 0x1000);
    Elf32DependencyGraph graph;
    graph.objects.resize(4);

    graph.objects[0].dependencies = {
        Elf32DependencyEdge{.requested_name = "one", .target_object = 1},
        Elf32DependencyEdge{.requested_name = "two", .target_object = 2},
    };
    graph.objects[1].dependencies = {
        Elf32DependencyEdge{.requested_name = "shared", .target_object = 3},
    };
    graph.objects[2].dependencies = {
        Elf32DependencyEdge{.requested_name = "shared", .target_object = 3},
    };
    graph.objects[3].dependencies = {
        Elf32DependencyEdge{.requested_name = "cycle", .target_object = 0},
    };

    if (!stage_init_array(memory, graph, 3, 0x1010, {0x3001U}) ||
        !stage_init_array(memory, graph, 1, 0x1020, {0U, 0x1001U}) ||
        !stage_init_array(memory, graph, 2, 0x1030,
                          {0xffffffffU, 0x2000U}) ||
        !stage_init_array(memory, graph, 0, 0x1040, {0x4000U})) {
        return fail("could not stage INIT_ARRAY planning graph");
    }

    std::array<std::uint8_t, 0x40> before{};
    if (!memory.read(0x1010, before)) {
        return fail("could not snapshot INIT_ARRAY planning bytes");
    }

    const auto plan = plan_elf32_init_array_calls(
        memory, graph, 0,
        Elf32InitPlanOptions{.max_objects = 4, .max_entries = 6});
    if (!plan || plan.calls.size() != 4) {
        return fail("dependency-first INIT_ARRAY plan did not produce four calls");
    }

    const std::array<std::size_t, 4> expected_objects{3, 1, 2, 0};
    const std::array<std::uint32_t, 4> expected_indices{0, 1, 1, 0};
    const std::array<std::uint32_t, 4> expected_functions{
        0x3001U, 0x1001U, 0x2000U, 0x4000U};
    for (std::size_t index = 0; index < plan.calls.size(); ++index) {
        if (plan.calls[index].object_index != expected_objects[index] ||
            plan.calls[index].array_index != expected_indices[index] ||
            plan.calls[index].function != expected_functions[index]) {
            return fail("INIT_ARRAY plan changed dependency/order/provenance semantics");
        }
    }

    std::array<std::uint8_t, 0x40> after{};
    if (!memory.read(0x1010, after) || after != before ||
        graph.objects[0].dependencies.size() != 2 ||
        graph.objects[3].dependencies.size() != 1) {
        return fail("INIT_ARRAY planning mutated guest memory or graph state");
    }
    return 0;
}

int test_init_plan_input_limits_and_decode_failures() {
    LinearGuestMemory memory(0x200, 0x1000);

    {
        Elf32DependencyGraph graph;
        graph.objects.resize(1);
        const auto invalid = plan_elf32_init_array_calls(
            memory, graph, 0,
            Elf32InitPlanOptions{.max_objects = 0, .max_entries = 0});
        if (invalid.error != Elf32InitPlanError::InvalidOptions ||
            !invalid.calls.empty()) {
            return fail("zero INIT_ARRAY object ceiling was not rejected");
        }

        const auto bad_root = plan_elf32_init_array_calls(
            memory, graph, 9,
            Elf32InitPlanOptions{.max_objects = 1, .max_entries = 0});
        if (bad_root.error != Elf32InitPlanError::InvalidRootObject ||
            !bad_root.failing_object.has_value() ||
            *bad_root.failing_object != 9) {
            return fail("invalid INIT_ARRAY root was not classified");
        }

        const auto empty = plan_elf32_init_array_calls(
            memory, graph, 0,
            Elf32InitPlanOptions{.max_objects = 1, .max_entries = 0});
        if (!empty || !empty.calls.empty()) {
            return fail("array-free INIT_ARRAY plan did not allow zero entry budget");
        }
    }

    {
        Elf32DependencyGraph graph;
        graph.objects.resize(2);
        graph.objects[0].dependencies = {
            Elf32DependencyEdge{.requested_name = "child", .target_object = 1},
        };
        const auto capped = plan_elf32_init_array_calls(
            memory, graph, 0,
            Elf32InitPlanOptions{.max_objects = 1, .max_entries = 0});
        if (capped.error != Elf32InitPlanError::ObjectLimitExceeded ||
            !capped.failing_object.has_value() ||
            *capped.failing_object != 1 || !capped.calls.empty()) {
            return fail("INIT_ARRAY unique-object ceiling was not enforced");
        }

        graph.objects[0].dependencies[0].target_object = 9;
        const auto bad_edge = plan_elf32_init_array_calls(
            memory, graph, 0,
            Elf32InitPlanOptions{.max_objects = 2, .max_entries = 0});
        if (bad_edge.error != Elf32InitPlanError::InvalidGraphEdge ||
            !bad_edge.failing_object.has_value() ||
            *bad_edge.failing_object != 0 || !bad_edge.calls.empty()) {
            return fail("invalid INIT_ARRAY graph edge was not rejected");
        }
    }

    {
        Elf32DependencyGraph graph;
        graph.objects.resize(1);
        if (!stage_init_array(memory, graph, 0, 0x1080,
                              {0x1111U, 0x2222U})) {
            return fail("could not stage INIT_ARRAY entry-limit fixture");
        }
        const auto capped = plan_elf32_init_array_calls(
            memory, graph, 0,
            Elf32InitPlanOptions{.max_objects = 1, .max_entries = 1});
        if (capped.error != Elf32InitPlanError::EntryLimitExceeded ||
            capped.decode_error !=
                Elf32FunctionArrayDecodeError::TooManyEntries ||
            !capped.failing_object.has_value() ||
            *capped.failing_object != 0 || !capped.calls.empty()) {
            return fail("INIT_ARRAY total-entry ceiling was not surfaced");
        }
    }

    {
        Elf32DependencyGraph graph;
        graph.objects.resize(1);
        graph.objects[0].linker_metadata.init_array =
            Elf32FunctionArrayMetadata{
                .guest_address = 0x9000,
                .size = 4,
            };
        const auto failed = plan_elf32_init_array_calls(
            memory, graph, 0,
            Elf32InitPlanOptions{.max_objects = 1, .max_entries = 1});
        if (failed.error != Elf32InitPlanError::DecodeFailed ||
            failed.decode_error != Elf32FunctionArrayDecodeError::ReadFailed ||
            !failed.failing_object.has_value() ||
            *failed.failing_object != 0 || !failed.calls.empty()) {
            return fail("INIT_ARRAY nested decoder failure was not surfaced");
        }
    }

    return 0;
}

}  // namespace

int main() {
    if (const int status = test_exact_decode_and_raw_sentinels(); status != 0) {
        return status;
    }
    if (const int status = test_entry_ceiling_preflights_before_read();
        status != 0) {
        return status;
    }
    if (const int status = test_invalid_size_and_range(); status != 0) {
        return status;
    }
    if (const int status = test_unreadable_and_empty_arrays(); status != 0) {
        return status;
    }
    if (const int status = test_init_plan_dependency_order_cycles_and_sentinels();
        status != 0) {
        return status;
    }
    if (const int status = test_init_plan_input_limits_and_decode_failures();
        status != 0) {
        return status;
    }
    return 0;
}
