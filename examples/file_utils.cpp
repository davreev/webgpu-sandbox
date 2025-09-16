#include "file_utils.hpp"

#include <fstream>

namespace dr
{

bool read_text_file(char const* const path, String& buffer)
{
    std::ifstream in{path, std::ios::in};
    if (!in)
        return false;

    using Iter = std::istreambuf_iterator<char>;
    buffer.assign(Iter{in}, {});
    return true;
}

bool append_text_file(char const* const path, String& buffer)
{
    std::ifstream in{path, std::ios::in};
    if (!in)
        return false;

    using Iter = std::istreambuf_iterator<char>;
    std::copy(Iter{in}, Iter{}, std::back_inserter(buffer));
    return true;
}

bool read_binary_file(char const* const path, DynamicArray<u8>& buffer)
{
    std::ifstream in{path, std::ios::in | std::ios::binary};
    if (!in)
        return false;

    using Iter = std::istreambuf_iterator<char>;
    buffer.assign(Iter{in}, {});
    return true;
}

bool append_binary_file(char const* const path, DynamicArray<u8>& buffer)
{
    std::ifstream in{path, std::ios::in | std::ios::binary};
    if (!in)
        return false;

    using Iter = std::istreambuf_iterator<char>;
    std::copy(Iter{in}, Iter{}, std::back_inserter(buffer));
    return true;
}

} // namespace dr