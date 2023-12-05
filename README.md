# WebGPU Sandbox

## Refs

- https://developer.chrome.com/blog/webgpu-cross-platform/
- https://eliemichel.github.io/LearnWebGPU/index.html

## Issues

- CMake install step for Dawn is broken
  - Must be included as a subproject which isn't ideal given its size
  - Maybe create a separate installable target that publicly links against Dawn? Would this install
    all necessary deps?