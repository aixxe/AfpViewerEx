#include <format>
#include <vector>
#include <ranges>
#include <algorithm>
#include <windows.h>
#include <Zydis/Zydis.h>

#include "memory.h"

auto find_generic(auto&& method, auto&& region,
    auto&& pattern, const bool silent) -> std::uint8_t*
{
    auto offset = std::optional<std::size_t> {};
    auto target = std::vector<std::optional<std::uint8_t>> {};

    for (auto&& range: pattern | std::views::split(' '))
    {
        auto hex = std::string_view { range };
        auto bin = std::optional<std::uint8_t> {};

        if (hex.empty())
            continue;

        if (!offset && hex.starts_with('[') && hex.ends_with(']'))
        {
            hex = hex.substr(1, hex.size() - 2);
            offset = target.size();
        }

        if (!hex.starts_with('?'))
            bin = std::stoi(hex.data(), nullptr, 16);

        target.emplace_back(bin);
    }

    auto result = method(region, target, [] (auto a, auto b)
        { return !b || a == *b; });

    if (result.empty() && !silent)
        throw std::runtime_error { "pattern not found" };

    return !result.empty() ? result.data() + offset.value_or(0): nullptr;
}

auto memory::find(std::span<std::uint8_t> region, std::string_view pattern, const bool silent) -> std::uint8_t*
    { return find_generic(std::ranges::search, region, pattern, silent); }

auto memory::rfind(std::span<std::uint8_t> region, std::string_view pattern, const bool silent) -> std::uint8_t*
    { return find_generic(std::ranges::find_end, region, pattern, silent); }

auto memory::follow(std::uint8_t* ptr, std::int64_t operand) -> std::uint8_t*
{
    auto decoder = ZydisDecoder {};
    auto status = ZydisDecoderInit
        (&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);

    if (!ZYAN_SUCCESS(status))
        throw std::runtime_error { "failed to initialize decoder" };

    auto instruction = ZydisDecodedInstruction {};
    auto operands = std::array<ZydisDecodedOperand, ZYDIS_MAX_OPERAND_COUNT> {};

    status = ZydisDecoderDecodeFull
        (&decoder, ptr, 32, &instruction, operands.data());

    if (!ZYAN_SUCCESS(status))
        throw std::runtime_error { "failed to decode instruction" };

    if (operand == -1)
    {
        operand = 0;

        auto const it = std::ranges::find_if(operands, [] (auto&& op)
            { return op.type == ZYDIS_OPERAND_TYPE_MEMORY &&
                    (op.mem.base == ZYDIS_REGISTER_RIP ||
                     op.mem.base == ZYDIS_REGISTER_EIP); });

        if (it != operands.end())
            operand = std::distance(operands.begin(), it);
    }

    auto rip = std::bit_cast<ZyanU64>(ptr);
    auto result = ZyanU64 {};

    status = ZydisCalcAbsoluteAddress
        (&instruction, &operands[operand], rip, &result);

    if (!ZYAN_SUCCESS(status))
        throw std::runtime_error { "failed to calculate absolute address" };

    return reinterpret_cast<std::uint8_t*>(result);
}