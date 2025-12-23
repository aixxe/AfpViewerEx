#include <windows.h>
#include <spdlog/spdlog.h>

#include "memory.h"
#include "capture_controller.h"

auto constexpr original_width = 1920;
auto constexpr original_height = 2160;
auto constexpr override_width = 3000;
auto constexpr override_height = 3000;

namespace { capture_controller* instance = nullptr; }

capture_controller::capture_controller() = default;
capture_controller::~capture_controller()
    { instance = nullptr; }

auto capture_controller::init(const modules::module_entry& bm2dx, const modules::module_entry& afp) -> void
{
    instance = this;

    afp::time_query_fn = reinterpret_cast<decltype(afp::time_query_fn)>
        (GetProcAddress(reinterpret_cast<HMODULE>(afp.base), "XCd229cc000072"));

    auto const init_addr = memory::find(bm2dx.region(), "48 8B 8E ? ? ? ? 48 8B 01 48 8D 55");
    auto const afp_frame_addr = memory::find(bm2dx.region(), "40 55 56 57 41 54 41 56 41 57");

    auto static init_hook = safetyhook::create_mid(init_addr,
        +[] (SafetyHookContext& ctx) -> void
    {
        auto const gfx = reinterpret_cast<gfx_context*>(ctx.rsi);

        instance->_d3d9_vmt = safetyhook::create_vmt(gfx->d3d9);

        instance->_draw_hook = *instance->_d3d9_vmt.hook_method(83, +[] (IDirect3DDevice9* self, D3DPRIMITIVETYPE type, UINT count, const void* data, UINT stride) -> HRESULT
            { return instance->on_primitive_draw(self, type, count, data, stride); });

        instance->_clear_hook = *instance->_d3d9_vmt.hook_method(43, +[] (IDirect3DDevice9* self, DWORD count, const D3DRECT* rects, DWORD flags, D3DCOLOR color, float z, DWORD stencil) -> HRESULT
            { return instance->on_clear(self, count, rects, flags, color, z, stencil); });

        instance->_viewport_hook = *instance->_d3d9_vmt.hook_method(47, +[] (IDirect3DDevice9* self, const D3DVIEWPORT9* vp) -> HRESULT
            { return instance->on_viewport_set(self, vp); });

        instance->_render_target_hook = *instance->_d3d9_vmt.hook_method(28, +[] (IDirect3DDevice9* self, UINT width, UINT height, D3DFORMAT format, D3DMULTISAMPLE_TYPE multisample, DWORD multisample_quality, BOOL lockable, IDirect3DSurface9** surface, HANDLE* shared) -> HRESULT
            { return instance->on_render_target_create(self, width, height, format, multisample, multisample_quality, lockable, surface, shared); });

        instance->_depth_stencil_hook = *instance->_d3d9_vmt.hook_method(29, +[] (IDirect3DDevice9* self, UINT width, UINT height, D3DFORMAT format, D3DMULTISAMPLE_TYPE multisample, DWORD multisample_quality, BOOL discard, IDirect3DSurface9** surface, HANDLE* shared) -> HRESULT
            { return instance->on_depth_stencil_create(self, width, height, format, multisample, multisample_quality, discard, surface, shared); });

        instance->_frame_saver = std::make_unique<async_frame_exporter>();
    });

    _afp_frame_hook = safetyhook::create_inline(afp_frame_addr, +[] (CAfpViewerScene* self) -> void
        { instance->on_afp_frame(self); });
}

auto capture_controller::basedir(std::filesystem::path dir) -> void
{
    _root_dir = std::move(dir);
    std::filesystem::create_directories(_root_dir);
}

auto capture_controller::on_afp_frame(CAfpViewerScene* self) -> void
{
    if (!_scene)
    {
        _scene = self;
        _scene->filtering(true);
    }

    if (_capture_state == capture_state::wait_next)
        _capture_state = capture_state::capture_queued;

    _afp_frame_hook.thiscall<void>(self);
}

auto capture_controller::on_primitive_draw(IDirect3DDevice9* self, D3DPRIMITIVETYPE type, UINT count, const void* data, UINT stride) -> HRESULT
{
    if (!_scene)
        return _draw_hook.thiscall<HRESULT>(self, type, count, data, stride);

    if (GetAsyncKeyState(VK_F12) && _capture_state == capture_state::user_idle)
    {
        _seen_frames.clear();

        _current_layer = _scene->layer_name();
        _current_dir = _root_dir / _current_layer;
        _capture_state = capture_state::capture_queued;

        std::filesystem::create_directories(_current_dir);
        spdlog::info("saving layer to '{}'", _current_dir.string());
    }

    if (_capture_state != capture_state::capture_queued || !_scene->layer || stride != 28)
        return _draw_hook.thiscall<HRESULT>(self, type, count, data, stride);

    // reset capture state when the user switches layers
    if (_scene->layer_name() != _current_layer)
    {
        _capture_state = capture_state::user_idle;
        return _draw_hook.thiscall<HRESULT>(self, type, count, data, stride);
    }

    auto const time = _scene->layer->time();
    auto const duration = _scene->layer->duration();

    if (!_seen_frames.contains(time))
    {
        auto const [width, height] = _scene->layer->dimensions();

        if (auto frame = _frame_capture.capture(self, time, width, height, _current_dir))
            _frame_saver->queue(std::move(*frame));
    }

    _seen_frames.insert(time);
    _capture_state = capture_state::wait_next;

    if (time >= duration)
        _capture_state = capture_state::user_idle;

    return _draw_hook.thiscall<HRESULT>(self, type, count, data, stride);
}

auto capture_controller::on_clear(IDirect3DDevice9* self, DWORD count, const D3DRECT* rects, DWORD flags, D3DCOLOR color, float z, DWORD stencil) -> HRESULT
    { return _clear_hook.thiscall<HRESULT>(self, count, rects, flags, D3DCOLOR_ARGB(0, 0, 0, 0), z, stencil); }

auto capture_controller::on_viewport_set(IDirect3DDevice9* self, const D3DVIEWPORT9* vp) -> HRESULT
{
    if (vp->Width != original_width || vp->Height != original_height)
        return _viewport_hook.thiscall<HRESULT>(self, vp);

    auto override = *vp;
    override.Width = override_width;
    override.Height = override_height;

    return _viewport_hook.thiscall<HRESULT>(self, &override);
}

auto capture_controller::on_render_target_create(IDirect3DDevice9* self, UINT width, UINT height, D3DFORMAT format, D3DMULTISAMPLE_TYPE multisample, DWORD multisample_quality, BOOL lockable, IDirect3DSurface9** surface, HANDLE* shared) -> HRESULT
{
    if (width == original_width && height == original_height)
    {
        width = override_width;
        height = override_height;
    }

    return _render_target_hook.thiscall<HRESULT>(self, width, height, format, multisample, multisample_quality, lockable, surface, shared);
}

auto capture_controller::on_depth_stencil_create(IDirect3DDevice9* self, UINT width, UINT height, D3DFORMAT format, D3DMULTISAMPLE_TYPE multisample, DWORD multisample_quality, BOOL discard, IDirect3DSurface9** surface, HANDLE* shared) -> HRESULT
{
    if (width == original_width && height == original_height)
    {
        width = override_width;
        height = override_height;
    }

    return _depth_stencil_hook.thiscall<HRESULT>(self, width, height, format, multisample, multisample_quality, discard, surface, shared);
}