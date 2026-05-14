#pragma once

#include <memory>

#include <dr/string.hpp>

#include "basic_types.hpp"

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

ImageAsset const* load_image_asset(char const* path, bool force_reload = false);
void release_image_asset(char const* path);

ShaderAsset const* load_shader_asset(char const* path, bool force_reload = false);
void release_shader_asset(char const* path);

void release_all_assets();

} // namespace wgpu::sandbox