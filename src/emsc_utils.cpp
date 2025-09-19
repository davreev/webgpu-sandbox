#include "emsc_utils.hpp"

#include <emscripten/emscripten.h>

// clang-format off

EM_JS(void, js_get_canvas_size, (int* dst_offset), {
    const dst = new Int32Array(HEAPU8.buffer, dst_offset, 2);
    dst[0] = Module.canvas.width;
    dst[1] = Module.canvas.height;
})

EM_JS(void, js_get_canvas_client_size, (int* dst_offset), {
    const dst = new Int32Array(HEAPU8.buffer, dst_offset, 2);
    dst[0] = Module.canvas.clientWidth;
    dst[1] = Module.canvas.clientHeight;
})

EM_JS(void, js_raise_event, (char const* name), {
    Module.eventTarget.dispatchEvent(new Event(UTF8ToString(name)));
})

EM_ASYNC_JS(void, js_wait_for_event, (char const* name), {
    await new Promise((resolve) => {
        Module.eventTarget.addEventListener(UTF8ToString(name), (e) => resolve(e), {once: true});
    });
})

// clang-format on

namespace wgpu::sandbox
{

void get_canvas_size(int& width, int& height)
{
    int dst[2];
    js_get_canvas_size(dst);
    width = dst[0];
    height = dst[1];
}

void get_canvas_client_size(int& width, int& height)
{
    int dst[2];
    js_get_canvas_client_size(dst);
    width = dst[0];
    height = dst[1];
}

void raise_event(char const* name) { js_raise_event(name); }

void wait_for_event(char const* name)
{
    js_wait_for_event(name);
    emscripten_sleep(0);
}

} // namespace wgpu::sandbox