#pragma once

#include <d3d9.h>
#include <filesystem>

#include "frame_exporter.h"

class frame_capture
{
    public:
        frame_capture() = default;

        auto capture(IDirect3DDevice9* device, int time, int width, int height, const std::filesystem::path& dir) -> std::optional<frame_data>;
};
