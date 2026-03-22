# FEM EM Studio (Native Starter)

Starter project for a desktop FEM EM application using C++ and Qt.

## Stack
- C++20
- Qt 6 Widgets
- QOpenGLWidget for the initial 3D viewport
- Native FEM core bridge (placeholder API)

## Build (Windows, Qt installed)
1. Configure:
   - `cmake -S . -B build -G "Ninja"`
2. Build:
   - `cmake --build build`
3. Run:
   - `build/FemEmStudio.exe`

## Next Steps
- Add Vulkan/DirectX backend adapters behind the same viewport interface.
- Replace placeholder FEM bridge with your real solver integration.
- Implement project tree grouped by assigned material.
- Add right-click material dialog from 3D selection.
