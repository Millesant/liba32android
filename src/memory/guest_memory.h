#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace liba32android::memory {

class GuestMemory {
public:
    virtual ~GuestMemory() = default;

    [[nodiscard]] virtual bool read(std::uint32_t address, std::span<std::uint8_t> output) const = 0;
    [[nodiscard]] virtual bool write(std::uint32_t address, std::span<const std::uint8_t> input) = 0;
};

// Minimal contiguous guest-address-space implementation used while M1/M2
// establish the generic memory seam. It intentionally makes no assumptions
// about low-VA host mappings or Dynarmic fastmem.
class LinearGuestMemory final : public GuestMemory {
public:
    explicit LinearGuestMemory(std::size_t size, std::uint32_t base = 0);

    [[nodiscard]] bool read(std::uint32_t address, std::span<std::uint8_t> output) const override;
    [[nodiscard]] bool write(std::uint32_t address, std::span<const std::uint8_t> input) override;

    [[nodiscard]] std::uint32_t base() const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;

private:
    [[nodiscard]] bool translate(std::uint32_t address, std::size_t length, std::size_t& offset) const noexcept;

    std::uint32_t base_{};
    std::vector<std::uint8_t> bytes_;
};

}  // namespace liba32android::memory
