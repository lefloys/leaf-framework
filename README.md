# leaf-framework

Leaf is the public framework core for Outposts of Odyssey. This directory is shaped as its own CMake project so it can move to a standalone repository cleanly.

It currently owns core utilities, math aliases, graphics/window abstraction types, system paths, and script/mod loading.

## Build

```powershell
cmake -S . -B out/build -DCMAKE_TOOLCHAIN_FILE=C:\Users\lefloysi\Desktop\GameDev\vcpkg\scripts\buildsystems\vcpkg.cmake
cmake --build out/build --config Debug --target leaf-core-example
```

From the OOO umbrella, the same example target is available as `leaf-core-example`.
