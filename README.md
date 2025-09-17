# WebGPU Sandbox

Experimenting with WebGPU as a cross-platform graphics API via
[wgpu-native](https://github.com/gfx-rs/wgpu-native)

## Build

This project can be built to run natively or in a web browser. Build instructions vary slightly
between the two targets.

### Native Build

Build via `cmake`

> ⚠️ Currently only tested with Clang and GCC. MSVC is not supported.

```sh
mkdir build
cmake -S . -B ./build -G <generator>
cmake --build ./build [--config <config>] [--target <target-name>]
```

### Web Build

Download the [Emscripten SDK](https://github.com/emscripten-core/emsdk) and dot source the
provided script to initialized the Emscripten toolchain

```sh
# Bash
EMSDK_DIR="absolute/path/of/emsdk/root"
. ./emsc-init.sh

# Powershell
$EMSDK_DIR="absolute/path/of/emsdk/root"
. ./emsc-init.ps1
```

Then build via `emcmake`

```sh
mkdir build
emcmake cmake -S . -B ./build -G <generator>
cmake --build ./build [--config <config>] [--target <target-name>]
```

Output can be served locally for testing e.g.

```sh
python -m http.server
```

> ⚠️ On Linux Chrome, WebGPU isn't enabled by default. To enable it, go to `chrome://flags` and set
> `Unsafe WebGPU Support` and `Vulkan` flags to `Enabled`.

## Refs

- https://www.w3.org/TR/webgpu
- https://www.w3.org/TR/WGSL
- https://webgpu-native.github.io/webgpu-headers
- https://github.com/eliemichel/WebGPU-distribution/tree/main
- https://eliemichel.github.io/LearnWebGPU/index.html
- https://developer.chrome.com/docs/web-platform/webgpu/build-app
- https://github.com/beaufortfrancois/webgpu-cross-platform-app
- https://github.com/kainino0x/webgpu-cross-platform-demo
- https://github.com/samdauwe/webgpu-native-examples
- https://github.com/ocornut/imgui/tree/master/examples/example_glfw_wgpu
- https://surma.dev/things/webgpu/
- https://sotrh.github.io/learn-wgpu
