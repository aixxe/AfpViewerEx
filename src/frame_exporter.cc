#include <png.h>
#include <chrono>
#include <fstream>
#include <spdlog/spdlog.h>

#include "scope_exit.h"
#include "frame_exporter.h"

async_frame_exporter::async_frame_exporter() = default;

async_frame_exporter::~async_frame_exporter()
{
    {
        auto lock = std::lock_guard { _mutex };
        _shutting_down = true;
    }

    _cv.notify_all();

    for (auto& thread: _workers)
        if (thread.joinable())
            thread.join();

    _workers.clear();
}

auto async_frame_exporter::queue(frame_data frame) -> void
{
    {
        auto lock = std::lock_guard { _mutex };
        _queue.push(std::move(frame));

        if (_active_threads == 0 && _workers.empty())
        {
            _workers.emplace_back(&async_frame_exporter::run_worker, this);
            ++_active_threads;
        }
        else if (_active_threads < MAX_THREADS && _queue.size() > static_cast<std::size_t>(_active_threads))
        {
            _workers.emplace_back(&async_frame_exporter::run_worker, this);
            ++_active_threads;
        }
    }

    _cv.notify_one();
}

auto async_frame_exporter::idle() const -> bool
{
    auto lock = std::lock_guard { _mutex };
    return _queue.empty() && _active_threads == 0;
}

auto async_frame_exporter::run_worker() -> void
{
    while (true)
    {
        auto frame = frame_data {};
        auto has_work = false;

        {
            auto lock = std::unique_lock { _mutex };
            auto const work = _cv.wait_for(lock, THREAD_KEEP_ALIVE, [this]
                { return !_queue.empty() || _shutting_down; });

            if (!_queue.empty())
            {
                frame = std::move(_queue.front());
                _queue.pop();
                has_work = true;
            }
            else if (_shutting_down)
                { break; }
            else if (!work)
                { --_active_threads; }
        }

        if (has_work)
            export_png(frame);
    }

    --_active_threads;
}

auto async_frame_exporter::export_png(const frame_data& frame) -> void
{
    auto const fp = fopen(frame.filename.c_str(), "wb");

    if (!fp)
        return spdlog::error("failed to open file for writing: {}", frame.filename);

    auto const fp_ = scope_exit { [&] { fclose(fp); } };
    auto png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);

    if (!png)
        return spdlog::error("failed to create PNG write struct");

    auto png_ = scope_exit { [&] { png_destroy_write_struct(&png, nullptr); } };
    auto info = png_create_info_struct(png);

    if (!info)
        return spdlog::error("failed to create PNG info struct");

    png_.replace([&] { png_destroy_write_struct(&png, &info); });

    if (setjmp(png_jmpbuf(png)))
        return spdlog::error("png write error");

    png_init_io(png, fp);
    png_set_compression_level(png, 1);
    png_set_IHDR(png, info, frame.width, frame.height, 8, PNG_COLOR_TYPE_RGBA,
        PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png, info);

    auto rows = std::vector<png_bytep>(frame.height);

    for (auto y = 0; y < frame.height; y++)
        rows[y] = const_cast<png_bytep>(&frame.pixels[y * frame.width * 4]);

    png_write_image(png, rows.data());
    png_write_end(png, nullptr);

    spdlog::info("frame saved to '{}'", frame.filename);
}
