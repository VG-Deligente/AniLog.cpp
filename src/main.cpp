#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <iostream>

int main() {
    // 1. Initialize the GLFW Windowing System
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    // 2. Create the Application Window
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Anime & Manga Tracker", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable VSync

    // 3. Initialize Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark(); // Dark Mode Theme

    // 4. Link ImGui to GLFW and OpenGL
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    // 5. The Main Application Loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents(); 

        // Start a new graphical frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // --- YOUR UI DASHBOARD STARTS HERE ---
        ImGui::Begin("Main Dashboard"); 
        ImGui::Text("Welcome to the Anime & Manga Tracker!");
        ImGui::Separator(); 
        
        if (ImGui::Button("Add New Anime Record")) {
            std::cout << "Add Record button clicked in console!\n";
        }
        ImGui::End();
        // --- YOUR UI DASHBOARD ENDS HERE ---

        // 6. Render the graphics to the screen
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f); 
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // 7. Clean up memory on exit
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}