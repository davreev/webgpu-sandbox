#pragma once

#include <memory>

#include <dr/basic_types.hpp>
#include <dr/string.hpp>

#include "../dr_shim.hpp"

namespace wgpu::sandbox
{

struct ImageAsset
{
    using Delete = void(u8*);
    std::unique_ptr<u8, Delete*> data{nullptr, nullptr};
    i32 width{};
    i32 height{};
    i32 stride{};
    i32 size() const { return width * height * stride; }
};

struct ShaderAsset
{
    String src{};
};

ImageAsset load_image_asset(char const* path);

ShaderAsset load_shader_asset(char const* path);

} // namespace wgpu::sandbox