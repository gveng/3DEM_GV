# FEM EM Studio (Native Starter)

Starter project for a desktop FEM EM application using C++ and Qt.

## Stack
- C++20
- Qt 6 Widgets
- QOpenGLWidget for the initial 3D viewport
- Native FEM core bridge (placeholder API)

## Build (Windows, Qt installed)
1. Install Qt 6.5+ with components:
   - `Qt 6.x -> MSVC or MinGW kit`
   - `Qt OpenGL module`
2. Configure (set Qt path):
   - `cmake -S . -B build -G "Ninja" -DCMAKE_PREFIX_PATH="C:/Qt/6.8.0/mingw_64"`
2. Build:
   - `cmake --build build`
3. Run:
   - `build/FemEmStudio.exe`

## Example Configure Paths
- MinGW kit: `C:/Qt/6.8.0/mingw_64`
- MSVC kit: `C:/Qt/6.8.0/msvc2022_64`

If CMake still cannot find Qt, set `Qt6_DIR` directly to the folder containing `Qt6Config.cmake`.

## Next Steps
- Add Vulkan/DirectX backend adapters behind the same viewport interface.
- Replace placeholder FEM bridge with your real solver integration.
- Implement project tree grouped by assigned material.
- Add right-click material dialog from 3D selection.
