#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <string>

enum Screen { LOGIN, SIGNUP, DASHBOARD };

int main() {
    if (!glfwInit()) return 1;
    GLFWwindow* window = glfwCreateWindow(1280, 720, "AniLog", NULL, NULL);
    glfwMakeContextCurrent(window);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    Screen currentScreen = LOGIN;
    // Input buffers
    char usernameInput[64] = "";
    char passwordInput[64] = "";

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame(); ImGui_ImplGlfw_NewFrame(); ImGui::NewFrame();

        ImGuiIO& io = ImGui::GetIO();

        if (currentScreen == LOGIN) {
            // Perfect centering for the login box
            ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(320, 300));

            ImGui::Begin("Auth", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
            
            ImGui::Text("Username");
            ImGui::InputText("##username", usernameInput, IM_ARRAYSIZE(usernameInput));

            ImGui::Spacing();
            ImGui::Text("Password");
            ImGui::InputText("##password", passwordInput, IM_ARRAYSIZE(passwordInput), ImGuiInputTextFlags_Password);

            ImGui::Spacing(); ImGui::Spacing();
            if (ImGui::Button("Log In", ImVec2(300, 40))) { currentScreen = DASHBOARD; }

            ImGui::Spacing();
            // Custom blue button for Sign Up
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.4f, 0.8f, 1.0f)); 
            if (ImGui::Button("Need an account? Sign Up", ImVec2(300, 30))) { currentScreen = SIGNUP; }
            ImGui::PopStyleColor(); // Return to default color

            ImGui::End();
        } 
        else if (currentScreen == SIGNUP) {
            // Placeholder for sign up screen
            ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
            ImGui::Begin("Sign Up", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::Text("Create your account");
            if (ImGui::Button("Back to Login")) { currentScreen = LOGIN; }
            ImGui::End();
        }
        else {
            // Dashboard (Fullscreen Layout)
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(200, io.DisplaySize.y));
            ImGui::Begin("Sidebar", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
            ImGui::Text("AniLog");
            if (ImGui::Button("Logout")) { currentScreen = LOGIN; }
            ImGui::End();
        }

        ImGui::Render();
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }
    // ... [Cleanup] ...
    return 0;
}