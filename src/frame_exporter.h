#pragma once

#include <queue>
#include <thread>
#include <condition_variable>

struct frame_data
{
    std::uint32_t width;
    std::uint32_t height;
    std::string filename;
    std::vector<std::uint8_t> pixels;
};

class async_frame_exporter
{
    public:
        async_frame_exporter();
        ~async_frame_exporter();

        async_frame_exporter(const async_frame_exporter&) = delete;
        async_frame_exporter& operator=(const async_frame_exporter&) = delete;
        async_frame_exporter(async_frame_exporter&&) = delete;
        async_frame_exporter& operator=(async_frame_exporter&&) = delete;

        auto queue(frame_data frame) -> void;

        [[nodiscard]] auto idle() const -> bool;

    private:
        auto static constexpr MAX_THREADS = 8;
        auto static constexpr THREAD_KEEP_ALIVE = std::chrono::milliseconds { 500 };

        mutable std::mutex _mutex;
        std::condition_variable _cv;
        std::queue<frame_data> _queue;
        std::vector<std::thread> _workers;
        std::atomic<int> _active_threads { 0 };
        std::atomic<bool> _shutting_down { false };

        auto run_worker() -> void;
        auto export_png(const frame_data& frame) -> void;
};
