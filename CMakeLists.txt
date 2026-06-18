cmake_minimum_required(VERSION 3.10)
project(AnimeTracker)

# Force modern C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find the GLFW library we installed via MSYS2
find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)

# Tell the compiler where the ImGui files are
include_directories(${CMAKE_SOURCE_DIR}/include/imgui)

# List every C++ file that needs to be compiled together
set(SOURCES
    src/main.cpp
    include/imgui/imgui.cpp
    include/imgui/imgui_draw.cpp
    include/imgui/imgui_tables.cpp
    include/imgui/imgui_widgets.cpp
    include/imgui/imgui_impl_glfw.cpp
    include/imgui/imgui_impl_opengl3.cpp
)

# Name the final app "AnimeTracker"
add_executable(${PROJECT_NAME} ${SOURCES})

# Link the graphics libraries to the app
target_link_libraries(${PROJECT_NAME} glfw OpenGL::GL)
