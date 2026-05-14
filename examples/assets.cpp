#include "assets.hpp"

#include <cassert>

#include <dr/app/asset_cache.hpp>
#include <dr/app/file_utils.hpp>

#include <stb_image.h>

namespace wgpu::sandbox
{
namespace
{

struct
{
    AssetCache<ImageAsset> images;
    AssetCache<ShaderAsset> shaders;
} state;

bool load_image(String const& path, ImageAsset& asset)
{
    constexpr int stride = 4;
    int width, height, src_stride;
    stbi_uc* const image_data = stbi_load(path.c_str(), &width, &height, &src_stride, stride);
    if (image_data == nullptr)
        return false;

    auto const free_image = [](u8* const data) { stbi_image_free(data); };
    asset = {{image_data, free_image}, width, height, stride};
    return true;
}

bool load_shader(String const& path, ShaderAsset& asset)
{
    return read_text_file(path.c_str(), asset.src);
}

} // namespace

ImageAsset const* load_image_asset(char const* const path, bool force_reload)
{
    return state.images.get(path, load_image, force_reload);
}

void release_image_asset(char const* const path) { state.images.remove(path); }

ShaderAsset const* load_shader_asset(char const* const path, bool force_reload)
{
    return state.shaders.get(path, load_shader, force_reload);
}

void release_shader_asset(char const* const path) { state.shaders.remove(path); }

void release_all_assets()
{
    state.images.clear();
    state.shaders.clear();
}

} // namespace wgpu::sandbox
