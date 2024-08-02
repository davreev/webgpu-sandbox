#pragma once

#include <cstddef>

#include <utility>

namespace wgpu
{

template <typename Func>
struct Deferred
{
    template <typename Func_>
    Deferred(Func_&& func) : func_(std::forward<Func_>(func))
    {
    }

    Deferred(Deferred const&) = delete;
    Deferred& operator=(Deferred const&) = delete;

    ~Deferred() { func_(); }

  private:
    Func func_;
};

template <typename Func>
[[nodiscard]] Deferred<Func> defer(Func&& func)
{
    return {std::forward<Func>(func)};
}

template <typename T>
struct Range
{
    T* ptr;
    std::size_t size;
    T* begin() const { return ptr; }
    T* end() const { return ptr + size; }
};

} // namespace wgpu