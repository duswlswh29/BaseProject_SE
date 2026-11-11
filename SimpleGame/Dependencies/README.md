# Bundled graphics dependencies

- GLFW 3.4: `GLFW/glfw-3.4/`, official release source including LICENSE.md.
  Source: https://github.com/glfw/glfw/releases/download/3.4/glfw-3.4.zip
- GLEW 2.1.0: `GLEW/`, matching generated headers, source and LICENSE.txt.
  Source: https://github.com/nigels-com/glew/releases/download/glew-2.1.0/glew-2.1.0.zip

The Visual Studio project builds these C sources directly with its own architecture
and CRT settings. No GLFW/GLEW DLL, installed SDK for these libraries, package
manager or network download is required when building from a clone.
Old FreeGLUT/GLEW prebuilt files are retained for history but are not linked.

Build: Windows, Visual Studio C++ tools v143, Windows 10/11 SDK, OpenGL 3.3
compatibility-capable GPU driver. Open SimpleGame.sln in the repository root,
select x64 (recommended) or Win32 and Debug/Release. Build and run.
Assets/Willowmere files are copied beside the executable by the existing target.
This remains a Windows game (GDI text, Win32 save/error handling); GLFW migration
does not imply Linux/macOS support for the rest of the game.

Update cadence remains approximately 16ms; rendering is continuous with swap
interval 0, as in the previous uncapped renderer. Minimized windows wait for
OS events. Key repeat is ignored, focus loss clears held inputs through the
existing Visibility handlers, and shutdown releases GPU resources before context
teardown. Compare Release x64 FPS/frame times with the existing profiler under
the same scene, resolution and driver settings. Runtime FPS comparison has not
been measured during migration.

Validation performed: all C/C++ sources compiled in Debug/Release x Win32/x64
without compiler diagnostics. All four link attempts were blocked by local
Visual Studio LNK1101 (MSPDB140.DLL version mismatch), including retry from
VsDevCmd. Full link and runtime FPS are therefore NOT verified. This local
toolchain issue was not hidden with linker ignores or system DLL changes.
