cmake_minimum_required(VERSION 3.15)

if(POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW)
endif()

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(AniLog)

# Force modern C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ANILOG_FETCH_GLFW "Download and build GLFW from source" OFF)

if(ANILOG_FETCH_GLFW)
    include(FetchContent)

    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.4
    )
    FetchContent_MakeAvailable(glfw)
else()
    # Find the GLFW library installed locally, such as through MSYS2.
    find_package(glfw3 REQUIRED)
endif()

find_package(OpenGL REQUIRED)

# Tell the compiler where the ImGui files are
include_directories(${CMAKE_SOURCE_DIR}/include/imgui)

# List every C++ file that needs to be compiled together
set(SOURCES
    src/main.cpp
    src/anilog_utils.cpp
    src/anilog_library.cpp
    src/anilog_add_edit.cpp
    src/anilog_statistics.cpp
    src/anilog_activity_log.cpp
    src/anilog_help.cpp
    include/imgui/imgui.cpp
    include/imgui/imgui_draw.cpp
    include/imgui/imgui_tables.cpp
    include/imgui/imgui_widgets.cpp
    include/imgui/imgui_impl_glfw.cpp
    include/imgui/imgui_impl_opengl3.cpp
)

# Name the final app "AniLog"
add_executable(${PROJECT_NAME} ${SOURCES})

if(WIN32)
    target_sources(${PROJECT_NAME} PRIVATE resources/anilog.rc)
    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

if(MSVC)
    target_link_options(${PROJECT_NAME} PRIVATE /ENTRY:mainCRTStartup)
endif()

# Link the graphics libraries to the app
target_link_libraries(${PROJECT_NAME} PRIVATE glfw OpenGL::GL)
