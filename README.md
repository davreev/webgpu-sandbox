# WebGPU Sandbox

Experimenting with WebGPU as a native graphics API

## Usage

Build via CMake as usual e.g.

```
mkdir build
cmake -S . -B ./build -G <generator> [-DCMAKE_EXPORT_COMPILE_COMMANDS=ON]
cmake --build ./build [--config <Debug|Release>]
```

## Refs

- https://www.w3.org/TR/webgpu
- https://www.w3.org/TR/WGSL
- https://developer.chrome.com/blog/webgpu-cross-platform/
- https://eliemichel.github.io/LearnWebGPU/index.html
- https://github.com/samdauwe/webgpu-native-examples