#pragma once

/*
    Injects dr namespace into wgpu::sandbox
*/

namespace dr
{
}

namespace wgpu::sandbox
{
inline namespace dr
{
using namespace ::dr;
}
} // namespace wgpu::sandbox