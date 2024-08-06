#pragma once

#include <fstream>
#include <string>

namespace wgpu::sandbox
{

bool read_text_file(char const* const path, std::string& buffer)
{
    std::ifstream in{path, std::ios::in};

    if (in)
    {
        using Iter = std::istreambuf_iterator<char>;
        buffer.assign(Iter{in}, {});
        return true;
    }
    else
    {
        return false;
    }
}

} // namespace wgpu::sandbox