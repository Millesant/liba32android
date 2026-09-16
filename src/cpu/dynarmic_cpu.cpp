#include "cpu/a32_cpu.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

#include "dynarmic/interface/A32/a32.h"
#include "dynarmic/interface/A32/arch_version.h"
#include "dynarmic/interface/A32/config.h"
#include "memory/guest_memory.h"

namespace liba32android::cpu {
namespace {

class Environment final : public Dynarmic::A32::UserCallbacks {
public:
    explicit Environment(memory::GuestMemory& memory) : memory_{memory} {}

    std::optional<std::uint32_t> MemoryReadCode(std::uint32_t vaddr) override {
        ++code_read_callbacks_;
        std::array<std::uint8_t, 4> bytes{};
        if (!memory_.read_code(vaddr, std::span<std::uint8_t>{bytes})) {
            memory_fault_ = true;
            return std::nullopt;
        }
        return static_cast<std::uint32_t>(bytes[0]) |
               static_cast<std::uint32_t>(bytes[1]) << 8 |
               static_cast<std::uint32_t>(bytes[2]) << 16 |
               static_cast<std::uint32_t>(bytes[3]) << 24;
    }

    std::uint8_t MemoryRead8(std::uint32_t vaddr) override {
        std::array<std::uint8_t, 1> bytes{};
        if (!read_bytes(vaddr, bytes)) {
            return 0;
        }
        return bytes[0];
    }

    std::uint16_t MemoryRead16(std::uint32_t vaddr) override {
        std::array<std::uint8_t, 2> bytes{};
        if (!read_bytes(vaddr, bytes)) {
            return 0;
        }
        return static_cast<std::uint16_t>(bytes[0]) |
               static_cast<std::uint16_t>(bytes[1]) << 8;
    }

    std::uint32_t MemoryRead32(std::uint32_t vaddr) override {
        std::array<std::uint8_t, 4> bytes{};
        if (!read_bytes(vaddr, bytes)) {
            return 0;
        }
        return static_cast<std::uint32_t>(bytes[0]) |
               static_cast<std::uint32_t>(bytes[1]) << 8 |
               static_cast<std::uint32_t>(bytes[2]) << 16 |
               static_cast<std::uint32_t>(bytes[3]) << 24;
    }

    std::uint64_t MemoryRead64(std::uint32_t vaddr) override {
        std::array<std::uint8_t, 8> bytes{};
        if (!read_bytes(vaddr, bytes)) {
            return 0;
        }

        std::uint64_t value = 0;
        for (std::size_t index = 0; index < bytes.size(); ++index) {
            value |= static_cast<std::uint64_t>(bytes[index]) << (index * 8);
        }
        return value;
    }

    void MemoryWrite8(std::uint32_t vaddr, std::uint8_t value) override {
        const std::array<std::uint8_t, 1> bytes{value};
        write_bytes(vaddr, bytes);
    }

    void MemoryWrite16(std::uint32_t vaddr, std::uint16_t value) override {
        const std::array<std::uint8_t, 2> bytes{
            static_cast<std::uint8_t>(value),
            static_cast<std::uint8_t>(value >> 8),
        };
        write_bytes(vaddr, bytes);
    }

    void MemoryWrite32(std::uint32_t vaddr, std::uint32_t value) override {
        const std::array<std::uint8_t, 4> bytes{
            static_cast<std::uint8_t>(value),
            static_cast<std::uint8_t>(value >> 8),
            static_cast<std::uint8_t>(value >> 16),
            static_cast<std::uint8_t>(value >> 24),
        };
        write_bytes(vaddr, bytes);
    }

    void MemoryWrite64(std::uint32_t vaddr, std::uint64_t value) override {
        std::array<std::uint8_t, 8> bytes{};
        for (std::size_t index = 0; index < bytes.size(); ++index) {
            bytes[index] = static_cast<std::uint8_t>(value >> (index * 8));
        }
        write_bytes(vaddr, bytes);
    }

    void InterpreterFallback(std::uint32_t, std::size_t) override {
        exception_raised_ = true;
    }

    void CallSVC(std::uint32_t) override {
        exception_raised_ = true;
    }

    void ExceptionRaised(std::uint32_t, Dynarmic::A32::Exception) override {
        exception_raised_ = true;
    }

    void AddTicks(std::uint64_t) override {}

    std::uint64_t GetTicksRemaining() override {
        return 1;
    }

    [[nodiscard]] bool exception_raised() const noexcept {
        return exception_raised_;
    }

    [[nodiscard]] bool memory_fault() const noexcept {
        return memory_fault_;
    }

    [[nodiscard]] std::size_t code_read_callbacks() const noexcept {
        return code_read_callbacks_;
    }

    [[nodiscard]] std::size_t data_read_callbacks() const noexcept {
        return data_read_callbacks_;
    }

    [[nodiscard]] std::size_t data_write_callbacks() const noexcept {
        return data_write_callbacks_;
    }

private:
    template <std::size_t Size>
    [[nodiscard]] bool read_bytes(std::uint32_t vaddr, std::array<std::uint8_t, Size>& bytes) {
        ++data_read_callbacks_;
        if (!memory_.read(vaddr, std::span<std::uint8_t>{bytes})) {
            memory_fault_ = true;
            return false;
        }
        return true;
    }

    template <std::size_t Size>
    void write_bytes(std::uint32_t vaddr, const std::array<std::uint8_t, Size>& bytes) {
        ++data_write_callbacks_;
        if (!memory_.write(vaddr, std::span<const std::uint8_t>{bytes})) {
            memory_fault_ = true;
        }
    }

    memory::GuestMemory& memory_;
    bool exception_raised_ = false;
    bool memory_fault_ = false;
    std::size_t code_read_callbacks_ = 0;
    std::size_t data_read_callbacks_ = 0;
    std::size_t data_write_callbacks_ = 0;
};

}  // namespace

ExecutionResult execute(memory::GuestMemory& memory, const ExecutionRequest& request) {
    Environment environment{memory};

    Dynarmic::A32::UserConfig config{};
    config.callbacks = &environment;
    config.arch_version = Dynarmic::A32::ArchVersion::v7;
    config.code_cache_size = 8 * 1024 * 1024;
    config.always_little_endian = true;

    const auto fastmem = memory.fastmem_base();
    if (fastmem.has_value()) {
        config.fastmem_pointer = *fastmem;
        config.recompile_on_fastmem_failure = true;
    }

    Dynarmic::A32::Jit jit{config};
    jit.Regs() = request.regs;
    jit.ExtRegs().fill(0);
    jit.Regs()[15] = request.entry_pc;

    // Start in AAPCS32 user mode. The T bit selects Thumb state.
    const std::uint32_t cpsr = request.instruction_set == InstructionSet::Thumb ? 0x30u : 0x10u;
    jit.SetCpsr(cpsr);

    std::size_t executed = 0;
    while (executed < request.instruction_count && !environment.exception_raised() && !environment.memory_fault()) {
        static_cast<void>(jit.Step());
        ++executed;
    }

    return ExecutionResult{
        .regs = jit.Regs(),
        .cpsr = jit.Cpsr(),
        .instructions_executed = executed,
        .exception_raised = environment.exception_raised(),
        .memory_fault = environment.memory_fault(),
        .fastmem_enabled = fastmem.has_value(),
        .code_read_callbacks = environment.code_read_callbacks(),
        .data_read_callbacks = environment.data_read_callbacks(),
        .data_write_callbacks = environment.data_write_callbacks(),
    };
}

}  // namespace liba32android::cpu
