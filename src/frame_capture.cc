#include <format>
#include <cstdint>
#include <algorithm>
#include <spdlog/spdlog.h>

#include "scope_exit.h"
#include "frame_capture.h"

auto frame_capture::capture(IDirect3DDevice9* device, int time, int width, int height, const std::filesystem::path& dir) -> std::optional<frame_data>
{
    if (!device)
        return std::nullopt;

    auto rt = (IDirect3DSurface9*) {};

    if (FAILED(device->GetRenderTarget(0, &rt)) || !rt)
        return std::nullopt;

    auto rt_ = scope_exit { [rt] { rt->Release(); } };
    auto desc = D3DSURFACE_DESC {};

    if (FAILED(rt->GetDesc(&desc)))
        return std::nullopt;

    auto sys = (IDirect3DSurface9*) {};

    if (FAILED(device->CreateOffscreenPlainSurface(desc.Width, desc.Height, desc.Format, D3DPOOL_SYSTEMMEM, &sys, nullptr)))
        return std::nullopt;

    auto sys_ = scope_exit { [sys] { sys->Release(); } };

    if (FAILED(device->GetRenderTargetData(rt, sys)))
        return std::nullopt;

    auto rect = D3DLOCKED_RECT {};

    if (FAILED(sys->LockRect(&rect, nullptr, D3DLOCK_READONLY)))
        return std::nullopt;

    auto rect_ = scope_exit { [sys] { sys->UnlockRect(); } };
    auto frame = frame_data {
        .width    = std::min<std::uint32_t>(width, static_cast<int>(desc.Width)),
        .height   = std::min<std::uint32_t>(height, static_cast<int>(desc.Height)),
        .filename = std::format("{}\\frame_{:05}.png", dir.string(), time),
    };

    frame.pixels.resize(frame.width * frame.height * 4);

    auto const src = static_cast<std::uint8_t*>(rect.pBits);
    auto const dst = frame.pixels.data();

    for (auto y = 0; y < frame.height; y++)
    {
        for (auto x = 0; x < frame.width; x++)
        {
            auto const src_idx = y * rect.Pitch + x * 4;
            auto const dst_idx = (y * frame.width + x) * 4;

            dst[dst_idx + 0] = src[src_idx + 2];
            dst[dst_idx + 1] = src[src_idx + 1];
            dst[dst_idx + 2] = src[src_idx + 0];
            dst[dst_idx + 3] = src[src_idx + 3];
        }
    }

    return frame;
}
