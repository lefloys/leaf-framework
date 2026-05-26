# Leaf Framework

Leaf is the C++23 framework used by Outposts of Odyssey. It owns the application loop, RML scene support, Lua scripting hooks, mod/prototype loading, localization, audio, and the Rutile graphics integration.

This directory is also a standalone CMake project, so Leaf can be built independently from the game.

## How To Compile

### Windows

These commands assume Visual Studio 2022 with the C++ workload, CMake, Ninja, Git, and PowerShell.

Install the Vulkan SDK from LunarG if you want to build or run the Vulkan backend. The vcpkg command below installs Vulkan headers and loader libraries, but the SDK provides the local Vulkan development/runtime environment expected by Rutile's Vulkan backend.

Install vcpkg:

```powershell
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
C:\vcpkg\bootstrap-vcpkg.bat
```

Install all dependencies with one command:

```powershell
C:\vcpkg\vcpkg.exe install --triplet x64-windows-static
```

Configure and build Leaf:

```powershell
cmake -S . -B out/build/x64-Debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows-static -DLEAF_BUILD_EXAMPLES=ON -DLEAF_BUILD_TESTS=ON
cmake --build out/build/x64-Debug --target leaf-framework
```

Build examples or tests:

```powershell
cmake --build out/build/x64-Debug --target leaf-framework-dev
ctest --test-dir out/build/x64-Debug --output-on-failure
```

From the Outposts of Odyssey umbrella project, Leaf is built through the game target and does not need a separate configure step.

### Linux

Install a compiler, CMake, Ninja, Git, pkg-config, X11/Wayland development packages for GLFW, PulseAudio/ALSA development packages for miniaudio, and the Vulkan SDK or distribution Vulkan development packages.

On Ubuntu/Debian, the system package baseline is:

```bash
sudo apt update
sudo apt install build-essential cmake ninja-build git pkg-config zip unzip tar \
  libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev \
  libwayland-dev wayland-protocols libxkbcommon-dev libpulse-dev libasound2-dev \
  vulkan-tools libvulkan-dev
```

Install vcpkg:

```bash
git clone https://github.com/microsoft/vcpkg.git ~/vcpkg
~/vcpkg/bootstrap-vcpkg.sh
```

Install all dependencies with one command:

```bash
~/vcpkg/vcpkg install --triplet x64-linux
```

Configure and build Leaf:

```bash
cmake -S . -B out/build/linux-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=$HOME/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-linux -DLEAF_BUILD_RUTILE_DX12=OFF -DLEAF_BUILD_EXAMPLES=ON -DLEAF_BUILD_TESTS=ON
cmake --build out/build/linux-debug --target leaf-framework
```

Build examples or tests:

```bash
cmake --build out/build/linux-debug --target leaf-framework-dev
ctest --test-dir out/build/linux-debug --output-on-failure
```

## Notes

Leaf requires `miniaudio` through `find_package(miniaudio REQUIRED)`. The local `cmake/Findminiaudio.cmake` module creates the `miniaudio::miniaudio` imported target for vcpkg installs that provide only `miniaudio.h`.

Rutile's Vulkan backend requires the Vulkan SDK, Vulkan headers, the Vulkan loader, glslang, SPIRV-Cross, and Vulkan Memory Allocator. The DirectX 12 backend uses the Windows SDK that ships with Visual Studio.
