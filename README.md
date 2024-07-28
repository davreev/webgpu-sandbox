# WebGPU Sandbox

Experimenting with WebGPU as a cross-platform graphics API

## Build

### Native

For native builds, use CMake

> ⚠️ Currently only tested with Clang and GCC. MSVC is not supported.

```sh
mkdir build
cmake -S . -B ./build -G <generator>
cmake --build ./build [--config <config>] [--target <target-name>]
```

### Web

For web builds, download the [Emscripten SDK](https://github.com/emscripten-core/emsdk) and dot source the appropriate setup script

```sh
# Bash
EMSDK_DIR="absolute/path/of/emsdk/root"
. ./emsc-setup.sh

# Powershell
$EMSDK_DIR="absolute/path/of/emsdk/root"
. ./emsc-setup.ps1
```

Then use CMake to build via `emcmake`

```sh
mkdir build 
emcmake cmake -S . -B ./build -G <generator>
cmake --build ./build [--config <config>] [--target <target-name>]
```

Output can be served locally for testing e.g.

```sh
python -m http.server
```

## Refs

- https://www.w3.org/TR/webgpu
- https://www.w3.org/TR/WGSL
- https://eliemichel.github.io/LearnWebGPU/index.html
- https://surma.dev/things/webgpu/
- https://developer.chrome.com/blog/webgpu-cross-platform/
- https://github.com/samdauwe/webgpu-native-examples
- https://github.com/kainino0x/webgpu-cross-platform-demo
- https://github.com/ocornut/imgui/tree/master/examples/example_glfw_wgpu