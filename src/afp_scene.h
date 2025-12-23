#pragma once

#include <utility>
#include <string_view>

namespace afp
    { extern std::int64_t (*time_query_fn) (std::int64_t, std::int32_t, ...); }

namespace BM2D
{
    struct CLayer
    {
        auto filtering(bool state) -> bool;
        auto dimensions() -> std::pair<std::int32_t, std::int32_t>;

        [[nodiscard]] auto time() const -> std::int32_t;
        [[nodiscard]] auto duration() const -> std::int32_t;

        /* 0x0000 */ std::uint8_t pad_0000[8];
        /* 0x0008 */ std::int64_t handle1;
        /* 0x0010 */ std::uint8_t pad_0010[512];
        /* 0x0210 */ std::int64_t handle2;
        /* 0x0218 */ std::uint8_t pad_0218[80];
    }; static_assert(sizeof(CLayer) == 0x268);
}

struct afp_info
{
    /* 0x0000 */ std::uint8_t pad_0000[16];
    /* 0x0010 */ std::int16_t width;
    /* 0x0012 */ std::int16_t height;
    /* 0x0014 */ std::uint8_t pad_0014[28];
    /* 0x0030 */ std::int16_t frames;
    /* 0x0032 */ std::uint8_t pad_0032[14];
}; static_assert(sizeof(afp_info) == 0x40);

enum class afp_category: std::int32_t
    { SYS, VER, ANM, INTR, QPR, ALL };

enum class afp_mode: std::int32_t
    { CMP, TEX, ALL };

struct CAfpViewerScene
{
    auto layer_name() const -> std::string_view;
    auto filtering(bool state) -> void;

    /* 0x000000 */ std::uint8_t pad_0000[152];
    /* 0x000098 */ char filenames[4096][257];
    /* 0x101098 */ char layernames[1104][256];
    /* 0x146098 */ std::uint8_t pad_146098[56];
    /* 0x1460D0 */ BM2D::CLayer* layer;
    /* 0x1460D8 */ std::int32_t file_count_total;
    /* 0x1460DC */ std::int32_t file_count_filtered;
    /* 0x1460E0 */ std::int32_t current_file;
    /* 0x1460E4 */ std::uint8_t pad_004C[4];
    /* 0x1460E8 */ std::int32_t layer_count_total;
    /* 0x1460EC */ std::int32_t current_layer;
    /* 0x1460F0 */ std::uint8_t pad_0058[4];
    /* 0x1460F4 */ bool display_info;
    /* 0x1460F5 */ std::uint8_t pad_005D[1];
    /* 0x1460F6 */ bool mc_disp;
    /* 0x1460F7 */ bool layer_loop;
    /* 0x1460F8 */ std::int32_t background;
    /* 0x1460FC */ std::int32_t filter;
    /* 0x146100 */ float scale;
    /* 0x146104 */ std::int32_t label_no;
    /* 0x146108 */ std::int32_t label_count;
    /* 0x14610C */ afp_category list_category;
    /* 0x146110 */ std::uint8_t pad_0078[4];
    /* 0x146114 */ afp_mode mode_type;
};

struct gfx_context
{
    /* 0x0000 */ char pad_0000[224];
    /* 0x00E0 */ struct IDirect3DDevice9* d3d9;
};
