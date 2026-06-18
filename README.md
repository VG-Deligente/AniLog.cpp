# AniLog.cpp

AniLog is a native desktop anime and manga tracker built with C++, CMake,
OpenGL, GLFW, and Dear ImGui. It stores your account, library, and activity log
locally on your computer.

> Status: Active development. The app is usable, but features and UI details may
> still change.

## Features

- Track anime and manga titles in a local library.
- Save progress, total episodes or chapters, ratings, and status.
- Edit and delete existing library entries.
- Search and filter by media type.
- View activity logs and statistics.
- Keep user data local instead of relying on a cloud service.

## Requirements

Before building AniLog, install these tools:

- Git
- CMake 3.10 or newer
- A C++17 compiler
- GLFW 3
- OpenGL development libraries

The project already includes the Dear ImGui source files under `include/imgui`,
so you do not need to install ImGui separately.

## Windows Setup

The easiest Windows setup is through MSYS2 UCRT64.

1. Install MSYS2 from <https://www.msys2.org/>.
2. Open the **MSYS2 UCRT64** terminal.
3. Update MSYS2:

   ```sh
   pacman -Syu
   ```

   If MSYS2 asks you to close the terminal, close it, reopen **MSYS2 UCRT64**,
   then run the same command again.

4. Install the build tools and GLFW:

   ```sh
   pacman -S --needed git mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-glfw
   ```

5. Clone the repository:

   ```sh
   git clone https://github.com/VG-Deligente/AniLog.cpp.git
   cd AniLog.cpp
   ```

6. Configure and build the project:

   ```sh
   cmake -S . -B build -G "MinGW Makefiles"
   cmake --build build
   ```

7. Run the app:

   ```sh
   ./build/AnimeTracker.exe
   ```

## Visual Studio Code Setup

You can also build the project from VS Code.

1. Install the **C/C++** extension from Microsoft.
2. Install the **CMake Tools** extension from Microsoft.
3. Open the cloned `AniLog.cpp` folder in VS Code.
4. Select your compiler kit when CMake Tools asks.
   - On Windows with MSYS2, choose the UCRT64 GCC compiler.
5. Run **CMake: Configure**.
6. Run **CMake: Build**.
7. Run the generated executable from the `build` folder.

## Project Structure

```text
AniLog.cpp/
├── include/imgui/     # Vendored Dear ImGui source files
├── src/main.cpp       # Main application source
├── CMakeLists.txt     # CMake build configuration
├── README.md          # Project instructions
└── LICENSE
```

## Local Data Files

AniLog saves user data in text files next to the executable. If you run the app
from the `build` folder, generated files may include:

- `users.txt`
- `<username>_library.txt`
- `<username>_logs.txt`
- `imgui.ini`

These files are local app data. They do not need to be committed to Git.

## Troubleshooting

### CMake says it cannot find GLFW

Make sure GLFW is installed for the same compiler and environment you are using.
For example, if you build with MSYS2 UCRT64, install the UCRT64 GLFW package:

```sh
pacman -S mingw-w64-ucrt-x86_64-glfw
```

### CMake cannot find `CMakeLists.txt`

The build file must be named exactly `CMakeLists.txt`. If your local copy has a
different capitalization, rename it before running CMake.

### The app builds but does not open

Make sure your graphics drivers support OpenGL and that you are running the app
from a desktop session, not from a headless terminal or remote shell without GUI
support.

### Windows cannot find DLL files

Run the app from the MSYS2 UCRT64 terminal, or add the UCRT64 `bin` folder to
your `PATH`. A common path is:

```text
C:\msys64\ucrt64\bin
```

## License

This project is licensed under the terms in [LICENSE](LICENSE).
