#include <format>
#include <ranges>
#include <algorithm>
#include <windows.h>
#include <winternl.h>

#include "modules.h"

auto narrow(const std::wstring_view input) -> std::string
{
    if (input.empty())
        return {};

    auto const wide_size = static_cast<int>(input.size());
    auto const narrow_size = WideCharToMultiByte(CP_UTF8, 0,
        input.data(), wide_size, nullptr, 0, nullptr, nullptr);

    if (narrow_size == 0)
        return {};

    auto buffer = std::string(narrow_size, '\0');
    auto const result = WideCharToMultiByte(CP_UTF8, 0, input.data(),
        wide_size, buffer.data(), narrow_size, nullptr, nullptr);

    if (result == 0)
        return {};

    return buffer;
}

auto modules::list() -> std::vector<module_entry>
{
    auto result = std::vector<module_entry> {};
    auto const peb = NtCurrentTeb()->ProcessEnvironmentBlock;

    if (!peb || !peb->Ldr)
        throw std::runtime_error { "failed to enumerate module list" };

    auto const list = &peb->Ldr->InMemoryOrderModuleList;

    for (auto entry = list->Flink; entry != list; entry = entry->Flink)
    {
        auto const module = CONTAINING_RECORD
            (entry, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);

        if (!module || !module->DllBase || !module->FullDllName.Buffer)
            continue;

        auto const base = static_cast<std::uint8_t*>(module->DllBase);
        auto const nt = nt_header(base);

        if (!nt || nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC)
            continue;

        result.emplace_back(module_entry {
            .base = base,
            .size = nt->OptionalHeader.SizeOfImage,
            .path = std::wstring {
                module->FullDllName.Buffer,
                module->FullDllName.Length / sizeof(wchar_t)
            },
        });
    }

    return result;
}

auto modules::nt_header(std::uint8_t* module) -> nt_header_type
{
    auto const dos = reinterpret_cast<dos_header_type>(module);

    if (dos->e_magic != IMAGE_DOS_SIGNATURE)
        return nullptr;

    auto const nt = reinterpret_cast<nt_header_type>(module + dos->e_lfanew);

    if (nt->Signature != IMAGE_NT_SIGNATURE)
        return nullptr;

    return nt;
}

auto modules::has_export(std::uint8_t* module, const std::string_view symbol) -> bool
{
    auto const nt = nt_header(module);

    if (!nt || nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC)
        return false;

    auto const [exports_va, exports_size] =
        nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];

    auto const exports = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>
        (module + exports_va);

    if (!exports_va || exports->NumberOfNames == 0)
        return false;

    auto const names = reinterpret_cast<DWORD*>
        (module + exports->AddressOfNames);

    for (auto i = 0ul; i < exports->NumberOfNames; ++i)
        if (symbol == reinterpret_cast<const char*>(module + names[i]))
            return true;

    return false;
}