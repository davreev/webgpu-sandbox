#pragma once

namespace wgpu::sandbox
{

void get_canvas_size(int& width, int& height);

void get_canvas_client_size(int& width, int& height);

void raise_event(char const* name);

void wait_for_event(char const* name);

} // namespace wgpu::sandbox
