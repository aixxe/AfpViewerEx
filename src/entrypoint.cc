#include <spdlog/spdlog.h>

#include "memory.h"
#include "modules.h"
#include "afp_scene.h"
#include "capture_controller.h"

auto init(std::filesystem::path&& cwd) -> void
{
    auto const modules = modules::list();

    auto const afp = std::ranges::find_if(modules, [] (auto&& entry)
        { return modules::has_export(entry.base, "XCd229cc000072"); });
    auto const bm2dx = std::ranges::find_if(modules, [] (auto&& entry)
        { return modules::has_export(entry.base, "dll_entry_main"); });

    if (bm2dx == modules.end())
        throw std::runtime_error { "bm2dx module not found" };
    if (afp == modules.end())
        throw std::runtime_error { "afp-core module not found" };

    auto static capture = capture_controller {};

    capture.init(*bm2dx, *afp);
    capture.basedir(cwd / "captures");
}

auto DllMain(HMODULE module, DWORD reason, LPVOID) -> BOOL
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(module);
        CreateThread(nullptr, 0, [] (LPVOID param) -> DWORD
        {
            auto const dll = static_cast<HMODULE>(param);

            auto path = std::wstring(MAX_PATH, L'\0');
            path.resize(GetModuleFileNameW(dll, path.data(), path.size()));

            try
                { init(std::filesystem::path { path }.parent_path()); }
            catch (std::exception const& e)
            {
                spdlog::error("afp hook init: {}", e.what());
                FreeLibraryAndExitThread(dll, EXIT_FAILURE);
            }

            return TRUE;
        }, module, 0, nullptr);
    }

    return TRUE;
}