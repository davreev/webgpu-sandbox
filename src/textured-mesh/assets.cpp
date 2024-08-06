#include "assets.hpp"

#include <cassert>

#include <stb_image.h>

#include "../shared/file_utils.hpp"

namespace wgpu::sandbox
{

ImageAsset load_image_asset(char const* const path)
{
    constexpr i32 stride = 4;
    i32 width, height, src_stride;
    auto const data = stbi_load(path, &width, &height, &src_stride, stride);
    assert(data);
    
    constexpr auto free_data = [](u8* data) { stbi_image_free(data); };
    return {{data, free_data}, width, height, stride};
}

ShaderAsset load_shader_asset(char const* const path)
{
    ShaderAsset result{};
    [[maybe_unused]] bool const ok = read_text_file(path, result.src);
    assert(ok);
    return result;
}

} // namespace wgpu::sandbox
