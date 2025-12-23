#pragma once

#include <d3d9.h>
#include <unordered_set>
#include <safetyhook.hpp>

#include "modules.h"
#include "afp_scene.h"
#include "frame_capture.h"
#include "frame_exporter.h"

class capture_controller
{
    public:
        capture_controller();
        ~capture_controller();

        capture_controller(const capture_controller&) = delete;
        capture_controller& operator=(const capture_controller&) = delete;
        capture_controller(capture_controller&&) = delete;
        capture_controller& operator=(capture_controller&&) = delete;

        auto init(modules::module_entry const& bm2dx, modules::module_entry const& afp) -> void;
        auto basedir(std::filesystem::path dir) -> void;

    private:
        enum class capture_state
            { user_idle, capture_queued, wait_next };

        auto on_afp_frame(CAfpViewerScene* self) -> void;
        auto on_primitive_draw(IDirect3DDevice9* self, D3DPRIMITIVETYPE type, UINT count, const void* data, UINT stride) -> HRESULT;
        auto on_clear(IDirect3DDevice9* self, DWORD count, const D3DRECT* rects, DWORD flags, D3DCOLOR color, float z, DWORD stencil) -> HRESULT;
        auto on_viewport_set(IDirect3DDevice9* self, const D3DVIEWPORT9* vp) -> HRESULT;
        auto on_render_target_create(IDirect3DDevice9* self, UINT width, UINT height, D3DFORMAT format, D3DMULTISAMPLE_TYPE multisample, DWORD multisample_quality, BOOL lockable, IDirect3DSurface9** surface, HANDLE* shared) -> HRESULT;
        auto on_depth_stencil_create(IDirect3DDevice9* self, UINT width, UINT height, D3DFORMAT format, D3DMULTISAMPLE_TYPE multisample, DWORD multisample_quality, BOOL discard, IDirect3DSurface9** surface, HANDLE* shared) -> HRESULT;

        CAfpViewerScene* _scene {};

        std::string _current_layer;
        frame_capture _frame_capture {};
        capture_state _capture_state { capture_state::user_idle };

        std::filesystem::path _root_dir {};
        std::filesystem::path _current_dir {};

        std::unordered_set<std::int32_t> _seen_frames {};
        std::unique_ptr<async_frame_exporter> _frame_saver {};

        safetyhook::VmtHook _d3d9_vmt {};
        safetyhook::VmHook _draw_hook {};
        safetyhook::VmHook _clear_hook {};
        safetyhook::VmHook _viewport_hook {};
        safetyhook::VmHook _render_target_hook {};
        safetyhook::VmHook _depth_stencil_hook {};
        safetyhook::InlineHook _afp_frame_hook {};
};
