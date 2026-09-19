#include <array>
#include <cstdint>
#include <iostream>
#include <span>
#include <string_view>

#include "cpu/a32_cpu.h"
#include "memory/guest_memory.h"

namespace {

using liba32android::cpu::ExecutionRequest;
using liba32android::cpu::ExecutionResult;
using liba32android::cpu::InstructionSet;
using liba32android::memory::LinearGuestMemory;

constexpr std::size_t kMemorySize = 4096;

bool load_code(LinearGuestMemory& memory, std::span<const std::uint8_t> code) {
    return memory.write(0, code);
}

bool execution_ok(const ExecutionResult& result, std::size_t expected_instructions) {
    return !result.exception_raised && !result.memory_fault &&
           result.instructions_executed == expected_instructions;
}

bool read_u32(const LinearGuestMemory& memory, std::uint32_t address, std::uint32_t& value) {
    std::array<std::uint8_t, 4> bytes{};
    if (!memory.read(address, bytes)) {
        return false;
    }

    value = static_cast<std::uint32_t>(bytes[0]) |
            static_cast<std::uint32_t>(bytes[1]) << 8 |
            static_cast<std::uint32_t>(bytes[2]) << 16 |
            static_cast<std::uint32_t>(bytes[3]) << 24;
    return true;
}

ExecutionRequest thumb_request(std::size_t instruction_count) {
    ExecutionRequest request{};
    request.instruction_set = InstructionSet::Thumb;
    request.instruction_count = instruction_count;
    return request;
}

int run_register_state() {
    // add r2, r0, r1
    constexpr std::array<std::uint8_t, 4> code{0x01, 0x20, 0x80, 0xE0};
    LinearGuestMemory memory{kMemorySize};
    if (!load_code(memory, code)) {
        return 1;
    }

    ExecutionRequest request{};
    request.regs[0] = 20;
    request.regs[1] = 22;
    const auto result = liba32android::cpu::execute(memory, request);
    if (!execution_ok(result, 1) || result.regs[2] != 42) {
        std::cerr << "register-state test failed: r2=" << result.regs[2] << '\n';
        return 1;
    }
    return 0;
}

int run_branch() {
    // b 0x8; mov r0,#1; mov r0,#42
    constexpr std::array<std::uint8_t, 12> code{
        0x00, 0x00, 0x00, 0xEA,
        0x01, 0x00, 0xA0, 0xE3,
        0x2A, 0x00, 0xA0, 0xE3,
    };
    LinearGuestMemory memory{kMemorySize};
    if (!load_code(memory, code)) {
        return 1;
    }

    ExecutionRequest request{};
    request.instruction_count = 2;
    const auto result = liba32android::cpu::execute(memory, request);
    if (!execution_ok(result, 2) || result.regs[0] != 42) {
        std::cerr << "branch test failed: r0=" << result.regs[0] << '\n';
        return 1;
    }
    return 0;
}

int run_call() {
    // bl 0x8; LR receives the ARM return address 0x4.
    constexpr std::array<std::uint8_t, 12> code{
        0x00, 0x00, 0x00, 0xEB,
        0x01, 0x00, 0xA0, 0xE3,
        0x2A, 0x00, 0xA0, 0xE3,
    };
    LinearGuestMemory memory{kMemorySize};
    if (!load_code(memory, code)) {
        return 1;
    }

    ExecutionRequest request{};
    request.instruction_count = 2;
    const auto result = liba32android::cpu::execute(memory, request);
    if (!execution_ok(result, 2) || result.regs[0] != 42 || result.regs[14] != 4) {
        std::cerr << "call test failed: r0=" << result.regs[0] << " lr=" << result.regs[14] << '\n';
        return 1;
    }
    return 0;
}

int run_memory_load_store() {
    // str r0,[r1]; ldr r2,[r1]
    constexpr std::array<std::uint8_t, 8> code{
        0x00, 0x00, 0x81, 0xE5,
        0x00, 0x20, 0x91, 0xE5,
    };
    LinearGuestMemory memory{kMemorySize};
    if (!load_code(memory, code)) {
        return 1;
    }

    ExecutionRequest request{};
    request.regs[0] = 0x12345678;
    request.regs[1] = 0x300;
    request.instruction_count = 2;
    const auto result = liba32android::cpu::execute(memory, request);

    std::uint32_t stored = 0;
    if (!read_u32(memory, 0x300, stored) || !execution_ok(result, 2) ||
        result.regs[2] != 0x12345678 || stored != 0x12345678) {
        std::cerr << "memory test failed: r2=0x" << std::hex << result.regs[2]
                  << " stored=0x" << stored << std::dec << '\n';
        return 1;
    }
    return 0;
}

int run_stack() {
    // str r0,[sp,#-4]!; ldr r1,[sp]
    constexpr std::array<std::uint8_t, 8> code{
        0x04, 0x00, 0x2D, 0xE5,
        0x00, 0x10, 0x9D, 0xE5,
    };
    LinearGuestMemory memory{kMemorySize};
    if (!load_code(memory, code)) {
        return 1;
    }

    ExecutionRequest request{};
    request.regs[0] = 0xCAFEBABE;
    request.regs[13] = 0x800;
    request.instruction_count = 2;
    const auto result = liba32android::cpu::execute(memory, request);

    std::uint32_t stored = 0;
    if (!read_u32(memory, 0x7FC, stored) || !execution_ok(result, 2) ||
        result.regs[13] != 0x7FC || result.regs[1] != 0xCAFEBABE || stored != 0xCAFEBABE) {
        std::cerr << "stack test failed: sp=0x" << std::hex << result.regs[13]
                  << " r1=0x" << result.regs[1] << " stored=0x" << stored << std::dec << '\n';
        return 1;
    }
    return 0;
}

int run_thumb_branch() {
    // b.n target; movs r0,#1; target: movs r0,#42; nop
    constexpr std::array<std::uint8_t, 8> code{
        0x00, 0xE0,
        0x01, 0x20,
        0x2A, 0x20,
        0x00, 0xBF,
    };
    LinearGuestMemory memory{kMemorySize};
    if (!load_code(memory, code)) {
        return 1;
    }

    const auto result = liba32android::cpu::execute(memory, thumb_request(2));
    if (!execution_ok(result, 2) || result.regs[0] != 42) {
        std::cerr << "Thumb branch test failed: r0=" << result.regs[0] << '\n';
        return 1;
    }
    return 0;
}

int run_thumb_call() {
    // bl target; the skipped mov is at the return address. Thumb LR keeps bit 0 set.
    constexpr std::array<std::uint8_t, 12> code{
        0x00, 0xF0, 0x02, 0xF8,
        0x01, 0x20,
        0x00, 0xBF,
        0x2A, 0x20,
        0x00, 0xBF,
    };
    LinearGuestMemory memory{kMemorySize};
    if (!load_code(memory, code)) {
        return 1;
    }

    const auto result = liba32android::cpu::execute(memory, thumb_request(2));
    if (!execution_ok(result, 2) || result.regs[0] != 42 || result.regs[14] != 5) {
        std::cerr << "Thumb call test failed: r0=" << result.regs[0]
                  << " lr=" << result.regs[14] << '\n';
        return 1;
    }
    return 0;
}

int run_thumb_memory_load_store() {
    // str r0,[r1]; ldr r2,[r1]; nop
    constexpr std::array<std::uint8_t, 6> code{
        0x08, 0x60,
        0x0A, 0x68,
        0x00, 0xBF,
    };
    LinearGuestMemory memory{kMemorySize};
    if (!load_code(memory, code)) {
        return 1;
    }

    auto request = thumb_request(2);
    request.regs[0] = 0x12345678;
    request.regs[1] = 0x300;
    const auto result = liba32android::cpu::execute(memory, request);

    std::uint32_t stored = 0;
    if (!read_u32(memory, 0x300, stored) || !execution_ok(result, 2) ||
        result.regs[2] != 0x12345678 || stored != 0x12345678) {
        std::cerr << "Thumb memory test failed: r2=0x" << std::hex << result.regs[2]
                  << " stored=0x" << stored << std::dec << '\n';
        return 1;
    }
    return 0;
}

int run_thumb_stack() {
    // push {r0}; ldr r1,[sp]; nop
    constexpr std::array<std::uint8_t, 6> code{
        0x01, 0xB4,
        0x00, 0x99,
        0x00, 0xBF,
    };
    LinearGuestMemory memory{kMemorySize};
    if (!load_code(memory, code)) {
        return 1;
    }

    auto request = thumb_request(2);
    request.regs[0] = 0xCAFEBABE;
    request.regs[13] = 0x800;
    const auto result = liba32android::cpu::execute(memory, request);

    std::uint32_t stored = 0;
    if (!read_u32(memory, 0x7FC, stored) || !execution_ok(result, 2) ||
        result.regs[13] != 0x7FC || result.regs[1] != 0xCAFEBABE || stored != 0xCAFEBABE) {
        std::cerr << "Thumb stack test failed: sp=0x" << std::hex << result.regs[13]
                  << " r1=0x" << result.regs[1] << " stored=0x" << stored << std::dec << '\n';
        return 1;
    }
    return 0;
}

int run_thumb_svc_exception() {
    // svc #0; nop
    constexpr std::array<std::uint8_t, 4> code{0x00, 0xDF, 0x00, 0xBF};
    LinearGuestMemory memory{kMemorySize};
    if (!load_code(memory, code)) {
        return 1;
    }

    const auto result = liba32android::cpu::execute(memory, thumb_request(1));
    if (!result.exception_raised || result.memory_fault || result.instructions_executed != 1) {
        std::cerr << "Thumb SVC test failed: executed=" << result.instructions_executed
                  << " exception=" << result.exception_raised
                  << " memory_fault=" << result.memory_fault << '\n';
        return 1;
    }
    return 0;
}

int run_instruction_fetch_fault() {
    LinearGuestMemory memory{kMemorySize};

    ExecutionRequest request{};
    request.entry_pc = static_cast<std::uint32_t>(kMemorySize - 2);
    const auto result = liba32android::cpu::execute(memory, request);

    if (!result.memory_fault || result.code_read_callbacks == 0) {
        std::cerr << "instruction-fetch fault test failed: memory_fault=" << result.memory_fault
                  << " code_reads=" << result.code_read_callbacks << '\n';
        return 1;
    }
    return 0;
}

int run_thumb_data_fault() {
    // ldr r0,[r1]; nop
    constexpr std::array<std::uint8_t, 4> code{0x08, 0x68, 0x00, 0xBF};
    LinearGuestMemory memory{kMemorySize};
    if (!load_code(memory, code)) {
        return 1;
    }

    auto request = thumb_request(1);
    request.regs[1] = static_cast<std::uint32_t>(kMemorySize - 2);
    const auto result = liba32android::cpu::execute(memory, request);

    if (!result.memory_fault || result.data_read_callbacks == 0) {
        std::cerr << "Thumb data-fault test failed: memory_fault=" << result.memory_fault
                  << " data_reads=" << result.data_read_callbacks << '\n';
        return 1;
    }
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr
            << "usage: cpu_execution "
               "<registers|branch|call|memory|stack|thumb_branch|thumb_call|"
               "thumb_memory|thumb_stack|thumb_svc|fetch_fault|thumb_data_fault>\n";
        return 2;
    }

    const std::string_view mode{argv[1]};
    if (mode == "registers") {
        return run_register_state();
    }
    if (mode == "branch") {
        return run_branch();
    }
    if (mode == "call") {
        return run_call();
    }
    if (mode == "memory") {
        return run_memory_load_store();
    }
    if (mode == "stack") {
        return run_stack();
    }
    if (mode == "thumb_branch") {
        return run_thumb_branch();
    }
    if (mode == "thumb_call") {
        return run_thumb_call();
    }
    if (mode == "thumb_memory") {
        return run_thumb_memory_load_store();
    }
    if (mode == "thumb_stack") {
        return run_thumb_stack();
    }
    if (mode == "thumb_svc") {
        return run_thumb_svc_exception();
    }
    if (mode == "fetch_fault") {
        return run_instruction_fetch_fault();
    }
    if (mode == "thumb_data_fault") {
        return run_thumb_data_fault();
    }

    std::cerr << "unknown mode: " << mode << '\n';
    return 2;
}
