#pragma once

#include <functional>

struct scope_exit
{
    using fn_type = std::function<void ()>;

    explicit scope_exit(fn_type&& fn):
        _fn { std::move(fn) } {}

    ~scope_exit() { if (_fn) _fn(); }

    scope_exit(const scope_exit&) = delete;
    scope_exit& operator=(const scope_exit&) = delete;
    scope_exit(scope_exit&&) = delete;
    scope_exit& operator=(scope_exit&&) = delete;

    auto reset() noexcept -> void
        { _fn = nullptr; }

    auto replace(fn_type&& fn) noexcept -> void
        { _fn = std::move(fn); }

    fn_type _fn;
};