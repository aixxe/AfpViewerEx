#include "afp_scene.h"

namespace afp
    { decltype(time_query_fn) time_query_fn = nullptr; }

auto inline get_vmt(void* instance) -> std::uintptr_t*
    { return *static_cast<std::uintptr_t**>(instance); }

auto inline get_vm(void* instance, const std::size_t index) -> void*
    { return reinterpret_cast<void*>(get_vmt(instance)[index]); }

namespace BM2D
{
    auto CLayer::filtering(const bool state) -> bool
    {
        auto constexpr on = 0x80000001ull;
        auto constexpr off = 0x80000000ull;

        using fn_type = std::uint64_t (__thiscall*) (CLayer*, std::uint64_t);
        auto const fn = reinterpret_cast<fn_type>(get_vm(this, 32));

        return fn(this, state ? on: off) == 0;
    }

    auto CLayer::dimensions() -> std::pair<std::int32_t, std::int32_t>
    {
        auto width = std::int32_t {};
        auto height = std::int32_t {};

        using fn_type = std::uint64_t (__thiscall*) (CLayer*, std::int32_t*, std::int32_t*);
        auto const fn = reinterpret_cast<fn_type>(get_vm(this, 39));

        if (fn(this, &width, &height) < 0)
            return { 0, 0 };

        return { width, height };
    }

    auto CLayer::time() const -> std::int32_t
    {
        auto result = std::int32_t {};
        afp::time_query_fn(handle2, 0x1010, &result);
        return result;
    }

    auto CLayer::duration() const -> std::int32_t
    {
        auto result = std::int32_t {};
        afp::time_query_fn(handle2, 0x1011, &result);
        return result;
    }
}

auto CAfpViewerScene::layer_name() const -> std::string_view
    { return layernames[current_layer]; }

auto CAfpViewerScene::filtering(const bool state) -> void
    { filter = layer->filtering(state); }
