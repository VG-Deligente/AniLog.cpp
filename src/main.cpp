// =============================================================================
//  main.cpp
//  ANILOG - Anime & Manga Media Tracker
//  COMP 003 | Final Laboratory Project
//
//  Tech Stack:
//    - Dear ImGui  : immediate-mode GUI library
//    - GLFW        : OS window and input
//    - OpenGL 3    : GPU rendering backend
//    - C++ STL     : vectors, strings, file streams, sorting
//
//  File structure:
//    main.cpp              - window setup, render loop, sidebar, login screen
//    anilog_globals.h      - shared enums, structs, extern declarations
//    anilog_utils.cpp      - global definitions, utilities, file I/O, auth
//    anilog_library.cpp    - TAB 1: AniDex library view
//    anilog_add_edit.cpp   - TAB 2: Add / Edit record form
//    anilog_statistics.cpp - TAB 3: Statistics + donut & horizontal bar charts
//    anilog_activity_log.cpp - TAB 4: Activity log
//    anilog_help.cpp       - TAB 5: Help guide
//
//  WHERE THE TEXT SIZES COME FROM
//    The five content tabs all size their text from the FONT_SCALE_* values in
//    anilog_globals.h, so changing those changes the whole content area at once.
//    This file (main.cpp) draws two things that live in their OWN windows and
//    therefore set their own scale directly: the left SIDEBAR and the LOGIN /
//    SIGNUP screen. Search this file for SetWindowFontScale to find and adjust
//    them. Keeping them a touch larger than 1.0 matches the enlarged tab text.
// =============================================================================

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#if defined(_WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <windows.h>
#endif
#include "anilog_globals.h"

// Auth helpers defined in anilog_utils.cpp but not in anilog_globals.h
// (only needed by main.cpp)
void loadUsers();
void loadLibrary();
bool loginUser(const string& username, const string& password);
bool registerUser(const string& username, const string& password);
void clearMessage();

#if defined(_WIN32)
// GLFW creates the window, but Windows needs explicit WM_SETICON messages for
// the title bar/taskbar to use the same icon embedded in resources/anilog.rc.
static void setWindowsAppIcon(GLFWwindow* window) {
    HINSTANCE instance = GetModuleHandleA(nullptr);
    HWND hwnd = glfwGetWin32Window(window);

    HICON largeIcon = LoadIconA(instance, "IDI_ANILOG");
    HICON smallIcon = static_cast<HICON>(LoadImageA(
        instance,
        "IDI_ANILOG",
        IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON),
        GetSystemMetrics(SM_CYSMICON),
        LR_DEFAULTCOLOR));

    if (largeIcon) {
        SendMessageA(hwnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(largeIcon));
        SetClassLongPtrA(hwnd, GCLP_HICON, reinterpret_cast<LONG_PTR>(largeIcon));
    }

    if (smallIcon || largeIcon) {
        HICON icon = smallIcon ? smallIcon : largeIcon;
        SendMessageA(hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(icon));
        SetClassLongPtrA(hwnd, GCLP_HICONSM, reinterpret_cast<LONG_PTR>(icon));
    }
}
#endif

int main() {
    // -- Window and graphics setup --
    // GLFW owns the native window and OpenGL context; ImGui is initialized
    // afterward so it can render into that context every frame.
    if (!glfwInit()) return 1;
    GLFWwindow* window = glfwCreateWindow(1280, 720, "AniLog - Media Tracker", NULL, NULL);
    if (!window) { glfwTerminate(); return 1; }
#if defined(_WIN32)
    setWindowsAppIcon(window);
#endif
    glfwMakeContextCurrent(window);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    // Global ImGui theme. Shared colors used across tabs live in
    // anilog_globals.h, while these values tune ImGui's base widgets.
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding   = 8.0f;
    style.FrameRounding    = 6.0f;
    style.GrabRounding     = 6.0f;
    style.ItemSpacing      = ImVec2(8.0f, 8.0f);
    style.FramePadding     = ImVec2(8.0f, 6.0f);
    style.Colors[ImGuiCol_WindowBg]       = ImVec4(0.13f, 0.13f, 0.16f, 1.0f);
    style.Colors[ImGuiCol_ChildBg]        = ImVec4(0.10f, 0.10f, 0.13f, 1.0f);
    style.Colors[ImGuiCol_PopupBg]        = ImVec4(0.15f, 0.15f, 0.18f, 1.0f);
    style.Colors[ImGuiCol_Button]         = ImVec4(0.18f, 0.35f, 0.58f, 1.0f);
    style.Colors[ImGuiCol_ButtonHovered]  = ImVec4(0.24f, 0.45f, 0.75f, 1.0f);
    style.Colors[ImGuiCol_ButtonActive]   = ImVec4(0.12f, 0.25f, 0.45f, 1.0f);
    style.Colors[ImGuiCol_Header]         = ImVec4(0.18f, 0.35f, 0.58f, 0.8f);
    style.Colors[ImGuiCol_HeaderHovered]  = ImVec4(0.24f, 0.45f, 0.75f, 0.8f);
    style.Colors[ImGuiCol_FrameBg]        = ImVec4(0.20f, 0.20f, 0.25f, 1.0f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.25f, 0.25f, 0.30f, 1.0f);

    // Users are loaded once on startup. A user's personal library/logs are
    // loaded only after successful login.
    loadUsers();

    char usernameInput[64] = "";
    char passwordInput[64] = "";
    bool showPassword = false;

    const ImVec4 COL_ACTIVE_BTN     = ImVec4(0.24f, 0.48f, 0.80f, 1.0f);
    const ImVec4 COL_ACTIVE_HOVERED = ImVec4(0.30f, 0.55f, 0.90f, 1.0f);

    // =========================================================================
    //  MAIN RENDER LOOP
    // =========================================================================
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGuiIO& io = ImGui::GetIO();

        // =====================================================================
        //  LOGIN / SIGNUP SCREEN
        // =====================================================================
        if (currentScreen == LOGIN || currentScreen == SIGNUP) {
            // Login and Signup share one centered modal-style window. The mode
            // changes labels and button behavior, but the layout stays identical.
            float winW = 600.0f, winH = 640.0f, elemW = 480.0f;
            float offsetX = (winW - elemW) * 0.5f;

            ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f),
                                    ImGuiCond_Always, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(winW, winH));
            ImGui::Begin(currentScreen == LOGIN ? "Login" : "Signup", nullptr,
                         ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

            ImGui::Spacing();
            ImGui::SetWindowFontScale(3.0f);
            float titleW = ImGui::CalcTextSize("ANILOG").x;
            ImGui::SetCursorPosX((winW - titleW) * 0.5f);
            ImGui::TextColored(ImVec4(0.35f, 0.65f, 1.0f, 1.0f), "ANILOG");

            // Login/Signup body text size (its own window - not FONT_SCALE_*).
            // Enlarged for readability; the 480px-wide fields fit it cleanly.
            ImGui::SetWindowFontScale(1.5f);
            ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
            ImGui::Dummy(ImVec2(0, 30));

            ImGui::SetCursorPosX(offsetX);
            ImGui::Text(currentScreen == LOGIN ? "Username" : "Choose Username");
            ImGui::SetCursorPosX(offsetX);
            ImGui::SetNextItemWidth(elemW);
            ImGui::InputText("##user", usernameInput, IM_ARRAYSIZE(usernameInput));
            ImGui::Spacing();

            ImGui::SetCursorPosX(offsetX);
            ImGui::Text(currentScreen == LOGIN ? "Password" : "Create Password");
            ImGui::SetCursorPosX(offsetX);
            float toggleBtnW = 80.0f;
            float inputH = ImGui::GetFrameHeight();
            ImGuiInputTextFlags passFlags = showPassword
                ? ImGuiInputTextFlags_None : ImGuiInputTextFlags_Password;
            ImGui::SetNextItemWidth(elemW - toggleBtnW - 8.0f);
            ImGui::InputText("##pass", passwordInput, IM_ARRAYSIZE(passwordInput), passFlags);
            ImGui::SameLine();
            if (ImGui::Button(showPassword ? "Hide" : "Show", ImVec2(toggleBtnW, inputH)))
                showPassword = !showPassword;

            if (!authMessage.empty()) {
                ImGui::Spacing();
                ImGui::SetCursorPosX(offsetX);
                ImVec4 msgColor = isAuthError
                    ? ImVec4(1.0f, 0.4f, 0.4f, 1.0f) : ImVec4(0.4f, 1.0f, 0.6f, 1.0f);
                ImGui::TextColored(msgColor, "%s", authMessage.c_str());
            }

            ImGui::Spacing(); ImGui::Spacing();
            ImGui::SetCursorPosX(offsetX);

            // Submit buttons call the auth helpers in anilog_utils.cpp. Those
            // helpers set authMessage/isAuthError, so the UI only displays state.
            if (currentScreen == LOGIN) {
                if (ImGui::Button("Log In", ImVec2(elemW, 50))) {
                    if (loginUser(usernameInput, passwordInput)) {
                        currentScreen = DASHBOARD;
                        currentTab    = LIBRARY;
                        resetForm();
                    }
                }
                ImGui::Spacing(); ImGui::SetCursorPosX(offsetX);
                if (ImGui::Button("Need an account? Sign Up", ImVec2(elemW, 40))) {
                    currentScreen = SIGNUP;
                    clearMessage();
                    usernameInput[0] = '\0';
                    passwordInput[0] = '\0';
                }
            } else {
                if (ImGui::Button("Create Account", ImVec2(elemW, 50))) {
                    if (registerUser(usernameInput, passwordInput)) {
                        currentScreen = LOGIN;
                        usernameInput[0] = '\0';
                        passwordInput[0] = '\0';
                    }
                }
                ImGui::Spacing(); ImGui::SetCursorPosX(offsetX);
                if (ImGui::Button("Back to Log In", ImVec2(elemW, 40))) {
                    currentScreen = LOGIN;
                    clearMessage();
                    usernameInput[0] = '\0';
                    passwordInput[0] = '\0';
                }
            }

            // Tagline
            ImGui::Dummy(ImVec2(0, 20));
            ImGui::SetWindowFontScale(1.0f);
            const char* tagline = "Never lose track of what you're watching again.";
            float tagW = ImGui::CalcTextSize(tagline).x;
            ImGui::SetCursorPosX((winW - tagW) * 0.5f);
            ImGui::TextColored(ImVec4(0.55f, 0.65f, 0.75f, 1.0f), "%s", tagline);
            ImGui::End();
        }

        // =====================================================================
        //  DASHBOARD
        // =====================================================================
        else if (currentScreen == DASHBOARD) {

            // -- SIDEBAR --
            // The sidebar is a fixed-width navigation window. It owns the main
            // tab switcher and the logout confirmation flow.
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(250, io.DisplaySize.y));
            ImGui::Begin("Sidebar", nullptr,
                         ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
            // Sidebar text size (its own window - not driven by FONT_SCALE_*).
            // Enlarged to stay in step with the bigger tab text. The nav buttons
            // are 230x45, which comfortably fits this size.
            ImGui::SetWindowFontScale(1.55f);

            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.35f, 0.65f, 1.0f, 1.0f), "ANILOG");
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "User: %s", loggedInUser.c_str());
            ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

            // Shared sidebar button behavior. If the Add/Edit form has text in
            // it, navigation asks for confirmation before discarding the buffers.
            auto sidebarBtn = [&](const char* label, DashboardTab tab) {
                bool isActive = (currentTab == tab);
                if (isActive) {
                    ImGui::PushStyleColor(ImGuiCol_Button,        COL_ACTIVE_BTN);
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, COL_ACTIVE_HOVERED);
                }
                if (ImGui::Button(label, ImVec2(230, 45))) {
                    bool formHasData = (currentTab == ADD_MEDIA || currentTab == EDIT_MEDIA)
                                    && string(inputTitle) != "";
                    if (formHasData && tab != currentTab) {
                        pendingNavAway = true;
                        pendingTab     = tab;
                        ImGui::OpenPopup("Unsaved Changes");
                    } else {
                        if (tab == ADD_MEDIA) resetForm();
                        currentTab = tab;
                    }
                }
                if (isActive) ImGui::PopStyleColor(2);
                ImGui::Spacing();
            };

            sidebarBtn("AniDex",       LIBRARY);
            sidebarBtn("Add Record",   ADD_MEDIA);
            sidebarBtn("Statistics",   STATISTICS);
            sidebarBtn("Activity Log", ACTIVITY_LOG);
            sidebarBtn("Help",         HELP);

            ImVec2 center = ImGui::GetMainViewport()->GetCenter();

            // Unsaved Changes popup
            // This protects users from losing a partially filled Add/Edit form.
            // pendingTab remembers where they were trying to go.
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            if (ImGui::BeginPopupModal("Unsaved Changes", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("You have unsaved changes.");
                ImGui::Text("Leave anyway and discard them?");
                ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
                ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.65f, 0.2f, 0.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.3f, 0.3f, 1.0f));
                if (ImGui::Button("Yes, Discard", ImVec2(130, 35))) {
                    resetForm();
                    currentTab     = pendingTab;
                    pendingNavAway = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::PopStyleColor(2);
                ImGui::SameLine(); ImGui::Spacing(); ImGui::SameLine();
                if (ImGui::Button("Keep Editing", ImVec2(130, 35))) {
                    pendingNavAway = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }

            // Logout button
            ImGui::SetCursorPosY(io.DisplaySize.y - 70);
            ImGui::Separator(); ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.3f, 0.3f, 1.0f));
            if (ImGui::Button("Logout", ImVec2(230, 40)))
                ImGui::OpenPopup("Logout Confirmation");
            ImGui::PopStyleColor(2);

            // Logout confirmation modal
            // Logout clears all session-specific memory so the next user starts
            // with a clean dashboard after logging in.
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            if (ImGui::BeginPopupModal("Logout Confirmation", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("Are you sure you want to logout?");
                ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
                ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.3f, 0.3f, 1.0f));
                if (ImGui::Button("Yes, Logout", ImVec2(120, 35))) {
                    logActivity("User Logout: " + loggedInUser + " signed out.");
                    currentScreen = LOGIN;
                    loggedInUser  = "";
                    currentLibrary.clear();
                    activityLogs.clear();
                    usernameInput[0] = '\0';
                    passwordInput[0] = '\0';
                    resetForm();
                    ImGui::CloseCurrentPopup();
                }
                ImGui::PopStyleColor(2);
                ImGui::SameLine(); ImGui::Spacing(); ImGui::SameLine();
                if (ImGui::Button("Cancel", ImVec2(120, 35)))
                    ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
            }

            ImGui::End(); // End sidebar

            // -- MAIN CONTENT AREA --
            // The content window fills all space to the right of the sidebar.
            // Individual tabs are rendered from separate source files.
            ImGui::SetNextWindowPos(ImVec2(250, 0));
            ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x - 250, io.DisplaySize.y));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24.0f, 22.0f));
            ImGui::Begin("Content", nullptr,
                         ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
            ImGui::PopStyleVar();
            ImGui::SetWindowFontScale(FONT_SCALE_BODY);

            // -- Delegate to tab files --
            if      (currentTab == LIBRARY)                  RenderLibraryTab(center);
            else if (currentTab == ADD_MEDIA ||
                     currentTab == EDIT_MEDIA)               RenderAddEditTab(center);
            else if (currentTab == STATISTICS)               RenderStatisticsTab();
            else if (currentTab == ACTIVITY_LOG)             RenderActivityLogTab();
            else if (currentTab == HELP)                     RenderHelpTab();

            ImGui::End(); // End content area
        }

        // -- Render frame --
        // ImGui builds draw commands above; OpenGL clears the screen and renders
        // those commands here, then GLFW swaps the completed frame onscreen.
        ImGui::Render();
        glClearColor(0.10f, 0.10f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    // -- Cleanup --
    // Shut down in the reverse order of initialization.
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
