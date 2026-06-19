// =============================================================================
//  ANILOG - Anime & Manga Media Tracker
//  COMP 003 | Final Laboratory Project
//
//  Tech Stack:
//    - Dear ImGui  : immediate-mode GUI library (draws every UI element each frame)
//    - GLFW        : opens the OS window and handles keyboard/mouse input
//    - OpenGL 3    : GPU backend that ImGui uses to actually paint pixels
//    - C++ STL     : vectors, strings, file streams, sorting
//
//  How the program works at a high level:
//    1. On startup, load all registered users from "users.txt".
//    2. Show a Login / Signup screen.
//    3. After login, load that user's library from "<username>_library.txt"
//       and their activity history from "<username>_logs.txt".
//    4. Enter the main render loop - every frame we clear the screen,
//       draw all ImGui windows, and swap the buffer to the display.
//    5. On logout or window close, all data is already saved to disk
//       (we save immediately after every change).
// =============================================================================

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <string>
#include <fstream>
#include <vector>
#include <sstream>
#include <ctime>
#include <algorithm>
#include <cctype>       // for tolower() used in case-insensitive search

using namespace std;

// =============================================================================
//  ENUMS - Named constants for program states so we avoid magic numbers
// =============================================================================

// Which screen is currently displayed to the user
enum Screen { LOGIN, SIGNUP, DASHBOARD };

// Which tab inside the dashboard is active
enum DashboardTab { LIBRARY, ADD_MEDIA, EDIT_MEDIA, ACTIVITY_LOG, STATISTICS };

// =============================================================================
//  DATA STRUCTURES - Blueprints for the data we store
// =============================================================================

// Holds one registered account
struct UserRecord {
    string username;
    string password;    // NOTE: stored as plain text (school project scope)
};

// Holds one anime or manga entry in the user's library
struct MediaRecord {
    string title;
    string type;            // "Anime" or "Manga"
    int    currentProgress; // Episodes watched or chapters read so far
    int    totalProgress;   // Total episodes or chapters in the series
    int    rating;          // User score, 1-5
    string status;          // "Watching", "Reading", "Completed", or "Dropped"
    string dateStarted;     // Date the user first added/started the record (YYYY-MM-DD)
    string dateFinished;    // Date it was marked Completed; empty if not yet done
    int    rereadCount;     // How many times the user rewatched/reread this title
};

// =============================================================================
//  GLOBAL STATE - Variables that are alive for the entire program lifetime
// =============================================================================

// Data collections
vector<UserRecord>  userDatabase;   // All registered accounts loaded from users.txt
vector<MediaRecord> currentLibrary; // The logged-in user's media entries
vector<string>      activityLogs;   // The logged-in user's activity history (strings)

// Session state
string loggedInUser = "";
string authMessage  = "";   // Message shown on the login/signup screen
bool   isAuthError  = false;// true = show message in red, false = show in green

// Which screen / tab we're on right now
Screen       currentScreen = LOGIN;
DashboardTab currentTab    = LIBRARY;

// -- Add / Edit form input buffers --
// ImGui writes directly into these char arrays and int variables
char inputTitle[128] = "";
int  inputTypeIndex  = 0;   // 0 = Anime, 1 = Manga
int  inputCurrent    = 0;   // Current progress value in the form
int  inputTotal      = 12;  // Total episodes/chapters value in the form
int  inputRating     = 3;   // Rating slider value (1-5)
int  inputStatusIndex= 0;   // 0 = Watching/Reading, 1 = Completed, 2 = Dropped
int  editingIndex    = -1;  // Index into currentLibrary of the record being edited (-1 = none)

// -- Library filter / search --
char searchBuffer[128]  = "";
int  currentFilterIndex = 0; // 0 = All, 1 = Anime only, 2 = Manga only

// -- UI state flags --
bool pendingNavAway    = false; // true when user clicked a sidebar tab mid-edit (triggers confirmation)
DashboardTab pendingTab = LIBRARY; // The tab they were trying to switch to
bool showRewatchConfirm = false;   // true when Rewatch/Reread confirmation popup should open
int  rewatchTargetIndex = -1;      // Index of the record the user wants to rewatch/reread

// -- String lookup tables used by dropdown menus --
const char* typeOptions[]       = { "Anime", "Manga" };
const char* animeStatusOptions[]= { "Watching", "Completed", "Dropped" };
const char* mangaStatusOptions[]= { "Reading",  "Completed", "Dropped" };
const char* filterOptions[]     = { "All Media", "Anime Only", "Manga Only" };

// =============================================================================
//  UTILITY FUNCTIONS - Small helpers used throughout the program
// =============================================================================

// Returns today's date as a "YYYY-MM-DD" string using the system clock
string getCurrentDate() {
    time_t now = time(0);
    tm* ltm = localtime(&now);
    char dateStr[11];
    snprintf(dateStr, sizeof(dateStr), "%04d-%02d-%02d",
             1900 + ltm->tm_year, 1 + ltm->tm_mon, ltm->tm_mday);
    return string(dateStr);
}

// Returns a lowercase copy of the given string (used for case-insensitive search)
string toLowerStr(const string& s) {
    string result = s;
    for (int i = 0; i < (int)result.size(); i++)
        result[i] = (char)tolower((unsigned char)result[i]);
    return result;
}

// Removes leading and trailing spaces from a string (used to validate title input)
string trimStr(const string& s) {
    int start = 0, end = (int)s.size() - 1;
    while (start <= end && s[start] == ' ') start++;
    while (end >= start && s[end]   == ' ') end--;
    return s.substr(start, end - start + 1);
}

// Writes an activity log entry - saves it in memory AND appends it to the log file
void logActivity(const string& action) {
    string entry = "[" + getCurrentDate() + "] " + action;
    activityLogs.push_back(entry);

    ofstream file(loggedInUser + "_logs.txt", ios::app);
    if (file.is_open()) {
        file << entry << "\n";
        file.close();
    }
}

// Builds a human-readable "diff" string describing what changed between two records.
// Used by the Edit form to produce detailed activity log entries (e.g., "Rating 3->5").
string buildChangeSummary(const MediaRecord& before, const MediaRecord& after) {
    string changes = "";

    if (before.title != after.title)
        changes += "Title [" + before.title + "] -> [" + after.title + "] | ";

    if (before.type != after.type)
        changes += "Type " + before.type + " -> " + after.type + " | ";

    if (before.currentProgress != after.currentProgress)
        changes += "Progress " + to_string(before.currentProgress)
                 + " -> " + to_string(after.currentProgress) + " | ";

    if (before.totalProgress != after.totalProgress)
        changes += "Total " + to_string(before.totalProgress)
                 + " -> " + to_string(after.totalProgress) + " | ";

    if (before.rating != after.rating)
        changes += "Rating " + to_string(before.rating)
                 + " -> " + to_string(after.rating) + " | ";

    if (before.status != after.status)
        changes += "Status " + before.status + " -> " + after.status + " | ";

    // Strip trailing " | " separator if present
    if (changes.size() >= 3 && changes.substr(changes.size() - 3) == " | ")
        changes = changes.substr(0, changes.size() - 3);

    return changes.empty() ? "No changes detected." : changes;
}

// =============================================================================
//  SORTING COMPARATORS - Passed to std::sort to reorder the library table
// =============================================================================

bool sortTitleAsc  (const MediaRecord& a, const MediaRecord& b) { return a.title < b.title; }
bool sortTitleDesc (const MediaRecord& a, const MediaRecord& b) { return a.title > b.title; }
bool sortRatingDesc(const MediaRecord& a, const MediaRecord& b) { return a.rating > b.rating; }
bool sortProgressDesc(const MediaRecord& a, const MediaRecord& b) {
    // Sort by completion percentage so 8/10 ranks above 5/100
    float pctA = (a.totalProgress > 0) ? (float)a.currentProgress / a.totalProgress : 0;
    float pctB = (b.totalProgress > 0) ? (float)b.currentProgress / b.totalProgress : 0;
    return pctA > pctB;
}

// =============================================================================
//  FILE HANDLING - Reading and writing persistent data to disk
// =============================================================================

// Loads all user accounts from "users.txt" into the userDatabase vector.
// Each line in the file is: username,password
void loadUsers() {
    ifstream file("users.txt");
    if (!file.is_open()) return;
    UserRecord temp;
    while (getline(file, temp.username, ',') && getline(file, temp.password)) {
        userDatabase.push_back(temp);
    }
    file.close();
}

// Appends a single new user to the end of "users.txt"
void saveUser(UserRecord u) {
    ofstream file("users.txt", ios::app);
    if (file.is_open()) {
        file << u.username << "," << u.password << "\n";
        file.close();
    }
}

// Loads the logged-in user's library and activity logs from their personal files.
// Called once right after a successful login.
// Safe version: wraps stoi() in try/catch so a corrupted file won't crash the program.
void loadLibrary() {
    currentLibrary.clear();
    activityLogs.clear();

    ifstream file(loggedInUser + "_library.txt");
    if (file.is_open()) {
        string line;
        while (getline(file, line)) {
            // Each line is a pipe-delimited record:
            // title|type|currentProgress|totalProgress|rating|status|dateStarted|dateFinished|rereadCount
            // We use '|' as the delimiter so that commas in titles don't break parsing.
            stringstream ss(line);
            MediaRecord m;
            string val;

            try {
                getline(ss, m.title,          '|');
                getline(ss, m.type,           '|');
                getline(ss, val,              '|'); m.currentProgress = stoi(val);
                getline(ss, val,              '|'); m.totalProgress   = stoi(val);
                getline(ss, val,              '|'); m.rating          = stoi(val);
                getline(ss, m.status,         '|');
                getline(ss, m.dateStarted,    '|');
                getline(ss, m.dateFinished,   '|');
                getline(ss, val);                   m.rereadCount     = stoi(val);

                // Clamp values that must be within valid ranges
                if (m.totalProgress   < 1) m.totalProgress   = 1;
                if (m.currentProgress < 0) m.currentProgress = 0;
                if (m.currentProgress > m.totalProgress) m.currentProgress = m.totalProgress;
                if (m.rating < 1) m.rating = 1;
                if (m.rating > 5) m.rating = 5;
                if (m.rereadCount < 0) m.rereadCount = 0;

                currentLibrary.push_back(m);
            } catch (...) {
                // Skip any line that can't be parsed cleanly
            }
        }
        file.close();
    }

    // Load activity history
    ifstream logFile(loggedInUser + "_logs.txt");
    if (logFile.is_open()) {
        string logLine;
        while (getline(logFile, logLine)) {
            activityLogs.push_back(logLine);
        }
        logFile.close();
    }
}

// Rewrites the entire library file from the current in-memory vector.
// Called after every change (add, edit, delete, progress update).
// Uses '|' as delimiter so titles containing commas are stored safely.
void saveLibrary() {
    ofstream file(loggedInUser + "_library.txt");
    if (file.is_open()) {
        for (int i = 0; i < (int)currentLibrary.size(); i++) {
            const MediaRecord& m = currentLibrary[i];
            file << m.title           << "|"
                 << m.type            << "|"
                 << m.currentProgress << "|"
                 << m.totalProgress   << "|"
                 << m.rating          << "|"
                 << m.status          << "|"
                 << m.dateStarted     << "|"
                 << m.dateFinished    << "|"
                 << m.rereadCount     << "\n";
        }
        file.close();
    }
}

// =============================================================================
//  AUTHENTICATION - Login, signup, and session helpers
// =============================================================================

// Clears the auth message banner shown below the login/signup fields
void clearMessage() { authMessage = ""; isAuthError = false; }

// Attempts to create a new account. Returns true on success.
// Validates: non-empty fields, minimum password length, no duplicate usernames.
bool registerUser(const string& username, const string& password) {
    string trimmedUser = trimStr(username);
    if (trimmedUser.empty() || password.empty()) {
        authMessage = "All fields are required."; isAuthError = true; return false;
    }
    if (password.length() < 6) {
        authMessage = "Password must be at least 6 characters."; isAuthError = true; return false;
    }
    string lowerNew = toLowerStr(trimmedUser);
    for (int i = 0; i < (int)userDatabase.size(); i++) {
        if (toLowerStr(userDatabase[i].username) == lowerNew) {
            authMessage = "Username already taken."; isAuthError = true; return false;
        }
    }
    UserRecord newUser;
    newUser.username = trimmedUser;
    newUser.password = password;
    userDatabase.push_back(newUser);
    saveUser(newUser);
    authMessage = "Account created! You can now log in."; isAuthError = false;
    return true;
}

// Attempts to log in. Returns true on success, sets loggedInUser and loads library.
bool loginUser(const string& username, const string& password) {
    string trimmedUser = trimStr(username);
    if (trimmedUser.empty() || password.empty()) {
        authMessage = "Please fill in both fields."; isAuthError = true; return false;
    }
    string lowerNew = toLowerStr(trimmedUser);
    for (int i = 0; i < (int)userDatabase.size(); i++) {
        if (toLowerStr(userDatabase[i].username) == lowerNew && userDatabase[i].password == password) {
            loggedInUser = userDatabase[i].username;
            loadLibrary();
            logActivity("User Login: " + loggedInUser + " signed in.");
            clearMessage();
            return true;
        }
    }
    authMessage = "Incorrect username or password."; isAuthError = true;
    return false;
}

// Resets all Add/Edit form fields back to their defaults.
// Called when switching to the Add tab or cancelling an edit.
void resetForm() {
    inputTitle[0]    = '\0';
    inputTypeIndex   = 0;
    inputCurrent     = 0;
    inputTotal       = 12;
    inputRating      = 3;
    inputStatusIndex = 0;
    editingIndex     = -1;
}

// =============================================================================
//  VALIDATION HELPERS
// =============================================================================

// Returns true if the library already contains a title matching the given string.
// Comparison is case-insensitive so "Attack on Titan" and "attack on titan" are treated as the same.
// Pass excludeIndex >= 0 when editing so the record being edited doesn't flag itself.
bool titleExists(const string& title, int excludeIndex = -1) {
    string lowerNew = toLowerStr(trimStr(title));
    for (int i = 0; i < (int)currentLibrary.size(); i++) {
        if (i == excludeIndex) continue;
        if (toLowerStr(currentLibrary[i].title) == lowerNew) return true;
    }
    return false;
}

// =============================================================================
//  MAIN PROGRAM - Window creation, ImGui setup, and the render loop
// =============================================================================

int main() {
    // -- Window and graphics setup --
    if (!glfwInit()) return 1;
    GLFWwindow* window = glfwCreateWindow(1280, 720, "AniLog - Media Tracker", NULL, NULL);
    if (!window) { glfwTerminate(); return 1; }
    glfwMakeContextCurrent(window);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    // -- Global ImGui visual style --
    // We use dark mode and override specific colors to match AniLog's theme.
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding   = 8.0f;
    style.FrameRounding    = 6.0f;
    style.GrabRounding     = 6.0f;
    style.ItemSpacing      = ImVec2(8.0f, 8.0f);   // Uniform spacing between items
    style.FramePadding     = ImVec2(8.0f, 6.0f);   // Padding inside input boxes / buttons

    // Flat dark gray background (replaces the previous gradient that hurt contrast)
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

    loadUsers(); // Load existing accounts from disk before showing the login screen

    // Login / signup form buffers (local to main because they are only needed here)
    char usernameInput[64] = "";
    char passwordInput[64] = "";
    bool showPassword = false;

    // Reusable color constants for sidebar active-tab highlight
    const ImVec4 COL_ACTIVE_BTN    = ImVec4(0.24f, 0.48f, 0.80f, 1.0f);
    const ImVec4 COL_ACTIVE_HOVERED= ImVec4(0.30f, 0.55f, 0.90f, 1.0f);
    const ImVec4 COL_DEFAULT_BTN   = ImVec4(0.18f, 0.35f, 0.58f, 1.0f);

    // =========================================================================
    //  MAIN RENDER LOOP
    //  Runs continuously until the user closes the window.
    //  Every iteration = one frame drawn on screen.
    // =========================================================================
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents(); // Process keyboard, mouse, window events from the OS

        // Start a new ImGui frame - must happen before any ImGui:: calls
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGuiIO& io = ImGui::GetIO(); // Contains display size, delta time, input state

        // =====================================================================
        //  LOGIN / SIGNUP SCREEN
        //  Shown when no user is logged in. Handles both screens in one block
        //  since they share the same layout - only labels and button actions differ.
        // =====================================================================
        if (currentScreen == LOGIN || currentScreen == SIGNUP) {
            float winW  = 600.0f, winH = 640.0f, elemW = 480.0f;
            float offsetX = (winW - elemW) * 0.5f;

            ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f),
                                    ImGuiCond_Always, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(winW, winH));
            ImGui::Begin(currentScreen == LOGIN ? "Login" : "Signup", nullptr,
                         ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

            // App title banner
            ImGui::Spacing();
            ImGui::SetWindowFontScale(3.0f);
            float titleW = ImGui::CalcTextSize("ANILOG").x;
            ImGui::SetCursorPosX((winW - titleW) * 0.5f);
            ImGui::TextColored(ImVec4(0.35f, 0.65f, 1.0f, 1.0f), "ANILOG");

            ImGui::SetWindowFontScale(1.4f);
            ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
            ImGui::Dummy(ImVec2(0, 30));

            // Username field
            ImGui::SetCursorPosX(offsetX);
            ImGui::Text(currentScreen == LOGIN ? "Username" : "Choose Username");
            ImGui::SetCursorPosX(offsetX);
            ImGui::SetNextItemWidth(elemW);
            ImGui::InputText("##user", usernameInput, IM_ARRAYSIZE(usernameInput));
            ImGui::Spacing();

            // Password field with Show/Hide toggle
            ImGui::SetCursorPosX(offsetX);
            ImGui::Text(currentScreen == LOGIN ? "Password" : "Create Password");
            ImGui::SetCursorPosX(offsetX);
            float toggleBtnW = 80.0f;
            float inputH = ImGui::GetFrameHeight();
            ImGuiInputTextFlags passFlags = showPassword
                ? ImGuiInputTextFlags_None
                : ImGuiInputTextFlags_Password;
            ImGui::SetNextItemWidth(elemW - toggleBtnW - 8.0f);
            ImGui::InputText("##pass", passwordInput, IM_ARRAYSIZE(passwordInput), passFlags);
            ImGui::SameLine();
            if (ImGui::Button(showPassword ? "Hide" : "Show", ImVec2(toggleBtnW, inputH)))
                showPassword = !showPassword;

            // Auth message (error in red, success in green)
            if (!authMessage.empty()) {
                ImGui::Spacing();
                ImGui::SetCursorPosX(offsetX);
                ImVec4 msgColor = isAuthError
                    ? ImVec4(1.0f, 0.4f, 0.4f, 1.0f)
                    : ImVec4(0.4f, 1.0f, 0.6f, 1.0f);
                ImGui::TextColored(msgColor, "%s", authMessage.c_str());
            }

            ImGui::Spacing(); ImGui::Spacing();
            ImGui::SetCursorPosX(offsetX);

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
            ImGui::End();
        }

        // =====================================================================
        //  DASHBOARD
        //  Main application screen after login. Consists of:
        //    - A fixed sidebar on the left (navigation + logout)
        //    - A main content area on the right that changes per tab
        // =====================================================================
        else if (currentScreen == DASHBOARD) {

            // -- SIDEBAR --
            // Fixed 250px panel on the left with navigation buttons.
            // Active tab button is highlighted in a brighter blue.
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(250, io.DisplaySize.y));
            ImGui::Begin("Sidebar", nullptr,
                         ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
            ImGui::SetWindowFontScale(1.4f);

            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.35f, 0.65f, 1.0f, 1.0f), "ANILOG");
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "User: %s", loggedInUser.c_str());
            ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

            // Helper lambda: draws a sidebar button that is highlighted when its tab is active.
            // When clicking a sidebar button while the Add/Edit form has unsaved input,
            // we set a pending flag instead of switching immediately, then confirm with the user.
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
                        // User is mid-edit - ask for confirmation before discarding
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

            sidebarBtn("AniDex",      LIBRARY);
            sidebarBtn("Add Record",  ADD_MEDIA);
            sidebarBtn("Statistics",  STATISTICS);
            sidebarBtn("Activity Log",ACTIVITY_LOG);

            // -- Unsaved Changes Popup --
            // Shown when user tries to navigate away from Add/Edit with a non-empty title.
            ImVec2 center = ImGui::GetMainViewport()->GetCenter();
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
            // -- Logout button (pinned to bottom of sidebar) --
            ImGui::SetCursorPosY(io.DisplaySize.y - 70);
            ImGui::Separator(); ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.3f, 0.3f, 1.0f));
            if (ImGui::Button("Logout", ImVec2(230, 40)))
                ImGui::OpenPopup("Logout Confirmation");
            ImGui::PopStyleColor(2);

            // Logout confirmation modal
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            if (ImGui::BeginPopupModal("Logout Confirmation", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("Are you sure you want to logout?");
                ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
                ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.3f, 0.3f, 1.0f));
                if (ImGui::Button("Yes, Logout", ImVec2(120, 35))) {
                    logActivity("User Logout: " + loggedInUser + " signed out.");
                    // Clear all session state
                    currentScreen    = LOGIN;
                    loggedInUser     = "";
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
            // Takes up the rest of the screen to the right of the sidebar.
            ImGui::SetNextWindowPos(ImVec2(250, 0));
            ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x - 250, io.DisplaySize.y));
            ImGui::Begin("Content", nullptr,
                         ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
            ImGui::SetWindowFontScale(1.4f);

            // =================================================================
            //  TAB 1: ANIDEX (Library)
            //  The main table of all media entries.
            //  Shows active (Watching/Reading/Dropped) records in the top table.
            //  Shows Completed records in a second table below.
            //  Shows Dropped records in a third table at the bottom.
            // =================================================================
            if (currentTab == LIBRARY) {
                ImGui::Text("AniDex - My Media Vault");
                ImGui::Separator(); ImGui::Spacing();

                // -- Search and Filter bar --
                ImGui::SetNextItemWidth(300);
                ImGui::InputTextWithHint("##search", "Search title...", searchBuffer, IM_ARRAYSIZE(searchBuffer));
                string searchStr = toLowerStr(searchBuffer); // lowercase for case-insensitive matching

                ImGui::SameLine();
                ImGui::SetNextItemWidth(160);
                ImGui::Combo("Filter", &currentFilterIndex, filterOptions, IM_ARRAYSIZE(filterOptions));

                ImGui::SameLine();
                ImGui::Text("  Sort:");
                ImGui::SameLine();
                if (ImGui::Button("A-Z"))      std::sort(currentLibrary.begin(), currentLibrary.end(), sortTitleAsc);
                ImGui::SameLine();
                if (ImGui::Button("Z-A"))      std::sort(currentLibrary.begin(), currentLibrary.end(), sortTitleDesc);
                ImGui::SameLine();
                if (ImGui::Button("Rating"))   std::sort(currentLibrary.begin(), currentLibrary.end(), sortRatingDesc);
                ImGui::SameLine();
                if (ImGui::Button("Progress")) std::sort(currentLibrary.begin(), currentLibrary.end(), sortProgressDesc);

                ImGui::Spacing();

                // Common table flags used by all three tables on this tab for visual consistency
                ImGuiTableFlags tableFlags = ImGuiTableFlags_Borders
                                           | ImGuiTableFlags_RowBg
                                           | ImGuiTableFlags_SizingStretchProp;

                // -- ACTIVE MEDIA TABLE (Watching / Reading) --
                // Shows every record that is NOT Completed and NOT Dropped.
                // Excludes Completed/Dropped so they appear only in their own sections below.
                ImGui::SetWindowFontScale(1.4f);
                ImGui::TextColored(ImVec4(0.35f, 0.65f, 1.0f, 1.0f), "Active");
                ImGui::SetWindowFontScale(1.4f);
                ImGui::Spacing();

                // Count active entries to display an empty state message
                int activeCount = 0;
                for (int i = 0; i < (int)currentLibrary.size(); i++) {
                    if (currentLibrary[i].status == "Completed" || currentLibrary[i].status == "Dropped") continue;
                    if (currentFilterIndex == 1 && currentLibrary[i].type != "Anime") continue;
                    if (currentFilterIndex == 2 && currentLibrary[i].type != "Manga") continue;
                    if (searchStr != "" && toLowerStr(currentLibrary[i].title).find(searchStr) == string::npos) continue;
                    activeCount++;
                }

                if (activeCount == 0) {
                    ImGui::TextColored(ImVec4(0.55f, 0.55f, 0.55f, 1.0f),
                        "No active records. Add one with 'Add Record'!");
                    ImGui::Spacing();
                } else if (ImGui::BeginTable("LibraryTable", 6, tableFlags)) {
                    ImGui::TableSetupColumn("Title",    ImGuiTableColumnFlags_WidthStretch, 2.5f);
                    ImGui::TableSetupColumn("Type",     ImGuiTableColumnFlags_WidthStretch, 1.0f);
                    ImGui::TableSetupColumn("Progress", ImGuiTableColumnFlags_WidthStretch, 1.2f);
                    ImGui::TableSetupColumn("Status",   ImGuiTableColumnFlags_WidthStretch, 1.5f);
                    ImGui::TableSetupColumn("Rating",   ImGuiTableColumnFlags_WidthStretch, 0.8f);
                    ImGui::TableSetupColumn("Actions",  ImGuiTableColumnFlags_WidthStretch, 2.5f);
                    ImGui::TableHeadersRow();

                    int deleteTarget = -1; // We defer deletion to avoid modifying the vector mid-loop

                    for (int i = 0; i < (int)currentLibrary.size(); i++) {
                        // Skip completed and dropped - they belong in the sections below
                        if (currentLibrary[i].status == "Completed") continue;
                        if (currentLibrary[i].status == "Dropped")   continue;
                        // Apply type filter
                        if (currentFilterIndex == 1 && currentLibrary[i].type != "Anime") continue;
                        if (currentFilterIndex == 2 && currentLibrary[i].type != "Manga") continue;
                        // Apply case-insensitive search filter
                        if (searchStr != "" && toLowerStr(currentLibrary[i].title).find(searchStr) == string::npos) continue;

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0); ImGui::TextWrapped("%s", currentLibrary[i].title.c_str());
                        ImGui::TableSetColumnIndex(1); ImGui::Text("%s", currentLibrary[i].type.c_str());
                        ImGui::TableSetColumnIndex(2); ImGui::Text("%d / %d", currentLibrary[i].currentProgress, currentLibrary[i].totalProgress);
                        ImGui::TableSetColumnIndex(3); ImGui::Text("%s", currentLibrary[i].status.c_str());
                        ImGui::TableSetColumnIndex(4); ImGui::Text("%d/5", currentLibrary[i].rating);
                        ImGui::TableSetColumnIndex(5);

                        ImGui::PushID(i);

                        // Quick-progress button: only shown when there is still progress left
                        if (currentLibrary[i].currentProgress < currentLibrary[i].totalProgress) {
                            string btnLabel = (currentLibrary[i].type == "Anime") ? "+1 Episode" : "+1 Chapter";
                            if (ImGui::Button(btnLabel.c_str(), ImVec2(110, 0))) {
                                currentLibrary[i].currentProgress++;
                                string unit = (currentLibrary[i].type == "Anime") ? "episode" : "chapter";
                                logActivity("Progress Updated: [" + currentLibrary[i].title
                                          + "] advanced to " + unit + " "
                                          + to_string(currentLibrary[i].currentProgress)
                                          + " of " + to_string(currentLibrary[i].totalProgress));
                                // Auto-complete when the user reaches the last episode/chapter
                                if (currentLibrary[i].currentProgress == currentLibrary[i].totalProgress) {
                                    currentLibrary[i].status       = "Completed";
                                    currentLibrary[i].dateFinished = getCurrentDate();
                                    logActivity("Status Changed: [" + currentLibrary[i].title + "] marked as Completed.");
                                }
                                saveLibrary();
                            }
                            ImGui::SameLine();
                        }

                        // Edit button - pre-fills the Edit form with this record's data
                        if (ImGui::Button("Edit")) {
                            editingIndex   = i;
                            snprintf(inputTitle, sizeof(inputTitle), "%s", currentLibrary[i].title.c_str());
                            inputTypeIndex = (currentLibrary[i].type == "Anime") ? 0 : 1;
                            inputCurrent   = currentLibrary[i].currentProgress;
                            inputTotal     = currentLibrary[i].totalProgress;
                            inputRating    = currentLibrary[i].rating;
                            if      (currentLibrary[i].status == "Completed") inputStatusIndex = 1;
                            else if (currentLibrary[i].status == "Dropped")   inputStatusIndex = 2;
                            else                                               inputStatusIndex = 0;
                            currentTab = EDIT_MEDIA;
                        }
                        ImGui::SameLine();

                        // Delete button - opens confirmation popup
                        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
                        string delPopupId = "DelConf_Active_" + to_string(i);
                        if (ImGui::Button("Delete")) {
                            ImGui::OpenPopup(delPopupId.c_str());
                        }
                        ImGui::PopStyleColor(2);

                        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
                        if (ImGui::BeginPopupModal(delPopupId.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                            ImGui::Text("Delete \"%s\"?", currentLibrary[i].title.c_str());
                            ImGui::Text("This action cannot be undone.");
                            ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
                            ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
                            if (ImGui::Button("Yes, Delete", ImVec2(120, 35))) {
                                deleteTarget = i;
                                ImGui::CloseCurrentPopup();
                            }
                            ImGui::PopStyleColor(2);
                            ImGui::SameLine(); ImGui::Spacing(); ImGui::SameLine();
                            if (ImGui::Button("Cancel", ImVec2(120, 35)))
                                ImGui::CloseCurrentPopup();
                            ImGui::EndPopup();
                        }

                        ImGui::PopID();
                    }
                    ImGui::EndTable();

                    // Perform the deferred deletion now that the loop is finished
                    if (deleteTarget >= 0) {
                        logActivity("Record Deleted: [" + currentLibrary[deleteTarget].title + "] removed from the library.");
                        currentLibrary.erase(currentLibrary.begin() + deleteTarget);
                        saveLibrary();
                    }
                }

                ImGui::Spacing(); ImGui::Spacing();

                // -- COMPLETED MEDIA TABLE --
                // Only shows records with status == "Completed".
                // Columns match the active table proportions.
                // Adds a Date Finished column since that data is now tracked.
                ImGui::SetWindowFontScale(1.4f);
                ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.6f, 1.0f), "Completed");
                ImGui::SetWindowFontScale(1.4f);
                ImGui::Spacing();

                int completedCount = 0;
                for (int i = 0; i < (int)currentLibrary.size(); i++) {
                    if (currentLibrary[i].status != "Completed") continue;
                    if (currentFilterIndex == 1 && currentLibrary[i].type != "Anime") continue;
                    if (currentFilterIndex == 2 && currentLibrary[i].type != "Manga") continue;
                    if (searchStr != "" && toLowerStr(currentLibrary[i].title).find(searchStr) == string::npos) continue;
                    completedCount++;
                }

                if (completedCount == 0) {
                    ImGui::TextColored(ImVec4(0.55f, 0.55f, 0.55f, 1.0f),
                        "No completed titles yet. Keep watching!");
                    ImGui::Spacing();
                } else if (ImGui::BeginTable("CompletedTable", 6, tableFlags)) {
                    ImGui::TableSetupColumn("Title",         ImGuiTableColumnFlags_WidthStretch, 2.5f);
                    ImGui::TableSetupColumn("Type",          ImGuiTableColumnFlags_WidthStretch, 1.0f);
                    ImGui::TableSetupColumn("Date Finished", ImGuiTableColumnFlags_WidthStretch, 1.5f);
                    ImGui::TableSetupColumn("Rating",        ImGuiTableColumnFlags_WidthStretch, 0.8f);
                    ImGui::TableSetupColumn("Rewatched",     ImGuiTableColumnFlags_WidthStretch, 1.0f);
                    ImGui::TableSetupColumn("Actions",       ImGuiTableColumnFlags_WidthStretch, 2.5f);
                    ImGui::TableHeadersRow();

                    for (int i = 0; i < (int)currentLibrary.size(); i++) {
                        if (currentLibrary[i].status != "Completed") continue;
                        if (currentFilterIndex == 1 && currentLibrary[i].type != "Anime") continue;
                        if (currentFilterIndex == 2 && currentLibrary[i].type != "Manga") continue;
                        if (searchStr != "" && toLowerStr(currentLibrary[i].title).find(searchStr) == string::npos) continue;

                        ImGui::PushID(200 + i);
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0); ImGui::TextWrapped("%s", currentLibrary[i].title.c_str());
                        ImGui::TableSetColumnIndex(1); ImGui::Text("%s", currentLibrary[i].type.c_str());
                        ImGui::TableSetColumnIndex(2);
                        ImGui::Text("%s", currentLibrary[i].dateFinished.empty() ? "-" : currentLibrary[i].dateFinished.c_str());
                        ImGui::TableSetColumnIndex(3); ImGui::Text("%d/5", currentLibrary[i].rating);
                        ImGui::TableSetColumnIndex(4); ImGui::Text("%d", currentLibrary[i].rereadCount);
                        ImGui::TableSetColumnIndex(5);

                        // Rewatch / Reread button - prompts confirmation before resetting progress
                        string rBtnLabel = (currentLibrary[i].type == "Anime") ? "Rewatch" : "Reread";
                        string rPopupId  = "RewatchConf_" + to_string(i);
                        if (ImGui::Button(rBtnLabel.c_str(), ImVec2(100, 0)))
                            ImGui::OpenPopup(rPopupId.c_str());

                        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
                        if (ImGui::BeginPopupModal(rPopupId.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                            ImGui::Text("Restart \"%s\"?", currentLibrary[i].title.c_str());
                            ImGui::Text("This will reset progress to 0 and move it back to Active.");
                            ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
                            if (ImGui::Button("Yes, Restart", ImVec2(130, 35))) {
                                currentLibrary[i].rereadCount++;
                                currentLibrary[i].currentProgress = 0;
                                currentLibrary[i].status = (currentLibrary[i].type == "Anime") ? "Watching" : "Reading";
                                currentLibrary[i].dateFinished = "";
                                logActivity("Record Restarted: [" + currentLibrary[i].title
                                          + "] restarted (rewatch/reread #"
                                          + to_string(currentLibrary[i].rereadCount) + ").");
                                saveLibrary();
                                ImGui::CloseCurrentPopup();
                            }
                            ImGui::SameLine(); ImGui::Spacing(); ImGui::SameLine();
                            if (ImGui::Button("Cancel", ImVec2(100, 35)))
                                ImGui::CloseCurrentPopup();
                            ImGui::EndPopup();
                        }

                        ImGui::SameLine();

                        // Edit button for completed records
                        if (ImGui::Button("Edit")) {
                            editingIndex   = i;
                            snprintf(inputTitle, sizeof(inputTitle), "%s", currentLibrary[i].title.c_str());
                            inputTypeIndex = (currentLibrary[i].type == "Anime") ? 0 : 1;
                            inputCurrent   = currentLibrary[i].currentProgress;
                            inputTotal     = currentLibrary[i].totalProgress;
                            inputRating    = currentLibrary[i].rating;
                            inputStatusIndex = 1; // Completed
                            currentTab = EDIT_MEDIA;
                        }

                        ImGui::PopID();
                    }
                    ImGui::EndTable();
                }

                ImGui::Spacing(); ImGui::Spacing();

                // -- DROPPED MEDIA TABLE --
                // Shows records the user gave up on.
                // Includes a "Resume" button to move them back to Active.
                ImGui::SetWindowFontScale(1.4f);
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.3f, 1.0f), "Dropped");
                ImGui::SetWindowFontScale(1.4f);
                ImGui::Spacing();

                int droppedCount = 0;
                for (int i = 0; i < (int)currentLibrary.size(); i++) {
                    if (currentLibrary[i].status != "Dropped") continue;
                    if (currentFilterIndex == 1 && currentLibrary[i].type != "Anime") continue;
                    if (currentFilterIndex == 2 && currentLibrary[i].type != "Manga") continue;
                    if (searchStr != "" && toLowerStr(currentLibrary[i].title).find(searchStr) == string::npos) continue;
                    droppedCount++;
                }

                if (droppedCount == 0) {
                    ImGui::TextColored(ImVec4(0.55f, 0.55f, 0.55f, 1.0f),
                        "Nothing dropped. Good discipline!");
                    ImGui::Spacing();
                } else if (ImGui::BeginTable("DroppedTable", 6, tableFlags)) {
                    ImGui::TableSetupColumn("Title",    ImGuiTableColumnFlags_WidthStretch, 2.5f);
                    ImGui::TableSetupColumn("Type",     ImGuiTableColumnFlags_WidthStretch, 1.0f);
                    ImGui::TableSetupColumn("Progress", ImGuiTableColumnFlags_WidthStretch, 1.2f);
                    ImGui::TableSetupColumn("Rating",   ImGuiTableColumnFlags_WidthStretch, 0.8f);
                    ImGui::TableSetupColumn("Started",  ImGuiTableColumnFlags_WidthStretch, 1.5f);
                    ImGui::TableSetupColumn("Actions",  ImGuiTableColumnFlags_WidthStretch, 2.5f);
                    ImGui::TableHeadersRow();

                    int droppedDeleteTarget = -1;

                    for (int i = 0; i < (int)currentLibrary.size(); i++) {
                        if (currentLibrary[i].status != "Dropped") continue;
                        if (currentFilterIndex == 1 && currentLibrary[i].type != "Anime") continue;
                        if (currentFilterIndex == 2 && currentLibrary[i].type != "Manga") continue;
                        if (searchStr != "" && toLowerStr(currentLibrary[i].title).find(searchStr) == string::npos) continue;

                        ImGui::PushID(400 + i);
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0); ImGui::TextWrapped("%s", currentLibrary[i].title.c_str());
                        ImGui::TableSetColumnIndex(1); ImGui::Text("%s", currentLibrary[i].type.c_str());
                        ImGui::TableSetColumnIndex(2); ImGui::Text("%d / %d", currentLibrary[i].currentProgress, currentLibrary[i].totalProgress);
                        ImGui::TableSetColumnIndex(3); ImGui::Text("%d/5", currentLibrary[i].rating);
                        ImGui::TableSetColumnIndex(4);
                        ImGui::Text("%s", currentLibrary[i].dateStarted.empty() ? "-" : currentLibrary[i].dateStarted.c_str());
                        ImGui::TableSetColumnIndex(5);

                        // Resume button - moves the record back to Watching/Reading
                        if (ImGui::Button("Resume", ImVec2(80, 0))) {
                            currentLibrary[i].status = (currentLibrary[i].type == "Anime") ? "Watching" : "Reading";
                            logActivity("Status Changed: [" + currentLibrary[i].title + "] resumed from Dropped.");
                            saveLibrary();
                        }
                        ImGui::SameLine();

                        // Edit button for dropped records
                        if (ImGui::Button("Edit")) {
                            editingIndex     = i;
                            snprintf(inputTitle, sizeof(inputTitle), "%s", currentLibrary[i].title.c_str());
                            inputTypeIndex   = (currentLibrary[i].type == "Anime") ? 0 : 1;
                            inputCurrent     = currentLibrary[i].currentProgress;
                            inputTotal       = currentLibrary[i].totalProgress;
                            inputRating      = currentLibrary[i].rating;
                            inputStatusIndex = 2; // Dropped
                            currentTab = EDIT_MEDIA;
                        }
                        ImGui::SameLine();

                        // Delete button for dropped records
                        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
                        string dDelPopupId = "DelConf_Dropped_" + to_string(i);
                        if (ImGui::Button("Delete")) {
                            ImGui::OpenPopup(dDelPopupId.c_str());
                        }
                        ImGui::PopStyleColor(2);

                        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
                        if (ImGui::BeginPopupModal(dDelPopupId.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                            ImGui::Text("Delete \"%s\"?", currentLibrary[i].title.c_str());
                            ImGui::Text("This action cannot be undone.");
                            ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
                            ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
                            if (ImGui::Button("Yes, Delete", ImVec2(120, 35))) {
                                droppedDeleteTarget = i;
                                ImGui::CloseCurrentPopup();
                            }
                            ImGui::PopStyleColor(2);
                            ImGui::SameLine(); ImGui::Spacing(); ImGui::SameLine();
                            if (ImGui::Button("Cancel", ImVec2(120, 35)))
                                ImGui::CloseCurrentPopup();
                            ImGui::EndPopup();
                        }

                        ImGui::PopID();
                    }
                    ImGui::EndTable();

                    if (droppedDeleteTarget >= 0) {
                        logActivity("Record Deleted: [" + currentLibrary[droppedDeleteTarget].title + "] removed from the library.");
                        currentLibrary.erase(currentLibrary.begin() + droppedDeleteTarget);
                        saveLibrary();
                    }
                }
            }

            // =================================================================
            //  TAB 2: ADD / EDIT MEDIA
            //  A form for creating a new record or modifying an existing one.
            //  Both share the same UI - the mode (Add vs Edit) determines behavior on Save.
            // =================================================================
            else if (currentTab == ADD_MEDIA || currentTab == EDIT_MEDIA) {
                ImGui::Text(currentTab == ADD_MEDIA ? "Add New Record" : "Edit Record");
                ImGui::Separator(); ImGui::Spacing();

                // -- Inline validation message --
                // We build this each frame so it always reflects the current form state.
                string validationMsg = "";

                string titleStr  = trimStr(string(inputTitle));
                bool   titleBlank= titleStr.empty();
                bool   titleDupe = !titleBlank && titleExists(titleStr, editingIndex);

                if (titleBlank && string(inputTitle) != "")
                    validationMsg = "Title cannot be only spaces.";
                else if (titleDupe)
                    validationMsg = "A record with this title already exists.";

                // Title input
                ImGui::SetNextItemWidth(420);
                ImGui::InputText("Title", inputTitle, IM_ARRAYSIZE(inputTitle));
                if (!validationMsg.empty())
                    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", validationMsg.c_str());

                ImGui::Spacing();

                // Category type (Anime / Manga) radio buttons with proper left padding
                ImGui::Text("Category Type:");
                ImGui::SameLine();
                ImGui::SetCursorPosX(200);
                ImGui::RadioButton("Anime", &inputTypeIndex, 0);
                ImGui::SameLine();
                ImGui::RadioButton("Manga", &inputTypeIndex, 1);

                ImGui::Spacing();

                // Current progress input - label changes based on selected type
                ImGui::SetCursorPosX(20);
                ImGui::SetNextItemWidth(180);
                ImGui::InputInt(inputTypeIndex == 0 ? "Current Progress (Episodes)" : "Current Progress (Chapters)", &inputCurrent);

                ImGui::SetCursorPosX(20);
                ImGui::SetNextItemWidth(180);
                ImGui::InputInt(inputTypeIndex == 0 ? "Total Episodes" : "Total Chapters", &inputTotal);

                // Clamp values to sane ranges every frame
                if (inputCurrent < 0)           inputCurrent = 0;
                if (inputTotal   < 1)           inputTotal   = 1;
                if (inputCurrent > inputTotal)  inputCurrent = inputTotal;

                // Warn the user if progress and total look swapped
                if (inputCurrent == inputTotal && inputTotal > 0)
                    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.6f, 1.0f),
                        "Progress matches total - saving will mark this as Completed.");

                ImGui::Spacing();

                ImGui::SetCursorPosX(20);
                ImGui::SetNextItemWidth(200);
                ImGui::SliderInt("Rating (1-5)", &inputRating, 1, 5);

                ImGui::SetCursorPosX(20);
                ImGui::SetNextItemWidth(220);
                const char** currentStatusOptions = (inputTypeIndex == 0) ? animeStatusOptions : mangaStatusOptions;
                ImGui::Combo("Status", &inputStatusIndex, currentStatusOptions, 3);

                ImGui::Spacing(); ImGui::Spacing();

                // -- Save and Cancel buttons --
                bool canSave = !titleBlank && !titleDupe;

                if (!canSave) {
                    // Visually dim the Save button when it can't be pressed
                    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
                }

                if (ImGui::Button("Save Record", ImVec2(200, 50)) && canSave) {
                    if (currentTab == ADD_MEDIA) {
                        // Build a brand new MediaRecord from form data
                        MediaRecord newMedia;
                        newMedia.title           = titleStr;
                        newMedia.type            = typeOptions[inputTypeIndex];
                        newMedia.currentProgress = inputCurrent;
                        newMedia.totalProgress   = inputTotal;
                        newMedia.rating          = inputRating;
                        newMedia.status          = currentStatusOptions[inputStatusIndex];
                        newMedia.dateStarted     = getCurrentDate();
                        newMedia.rereadCount     = 0;

                        // If progress already matches the total, auto-complete the record
                        // (matches the on-screen hint and the +1 Episode/Chapter button behavior)
                        if (inputCurrent == inputTotal) {
                            newMedia.status       = "Completed";
                            newMedia.dateFinished = getCurrentDate();
                        } else {
                            newMedia.dateFinished = "";
                        }

                        currentLibrary.push_back(newMedia);
                        logActivity("Record Added: [" + newMedia.title + "] added to the library. "
                                  + "Type: " + newMedia.type + ", Status: " + newMedia.status
                                  + ", Rating: " + to_string(newMedia.rating));
                    } else {
                        // Guard against an invalid editingIndex (E5 fix)
                        if (editingIndex >= 0 && editingIndex < (int)currentLibrary.size()) {
                            // Snapshot the "before" state so we can log what changed
                            MediaRecord before = currentLibrary[editingIndex];

                            // Apply the edited values
                            currentLibrary[editingIndex].title           = titleStr;
                            currentLibrary[editingIndex].type            = typeOptions[inputTypeIndex];
                            currentLibrary[editingIndex].currentProgress = inputCurrent;
                            currentLibrary[editingIndex].totalProgress   = inputTotal;
                            currentLibrary[editingIndex].rating          = inputRating;
                            currentLibrary[editingIndex].status          = currentStatusOptions[inputStatusIndex];

                            // If progress already matches the total, auto-complete the record
                            // (matches the on-screen hint and the +1 Episode/Chapter button behavior)
                            if (inputCurrent == inputTotal) {
                                currentLibrary[editingIndex].status = "Completed";
                                if (currentLibrary[editingIndex].dateFinished.empty())
                                    currentLibrary[editingIndex].dateFinished = getCurrentDate();
                            } else {
                                // Progress no longer matches total - record can't be Completed
                                currentLibrary[editingIndex].dateFinished = "";
                            }

                            // Build and log a detailed change summary
                            string changes = buildChangeSummary(before, currentLibrary[editingIndex]);
                            logActivity("Record Updated: [" + titleStr + "] - " + changes);
                        }
                    }
                    saveLibrary();
                    currentTab = LIBRARY;
                    resetForm();
                }

                if (!canSave) ImGui::PopStyleColor(3);

                ImGui::SameLine();
                if (ImGui::Button("Cancel", ImVec2(100, 50))) {
                    currentTab = LIBRARY;
                    resetForm();
                }
            }

            // =================================================================
            //  TAB 3: STATISTICS
            //  Aggregates and displays totals, averages, and personal records
            //  derived from the user's library.
            // =================================================================
            else if (currentTab == STATISTICS) {
                ImGui::Text("Statistics");
                ImGui::Separator(); ImGui::Spacing();

                // -- Aggregate all stats in a single pass over the library --
                int totalAnime = 0, totalManga = 0;
                int epsWatched = 0, chsRead = 0;
                int completedCnt = 0, droppedCnt = 0, watchingCnt = 0, readingCnt = 0;
                int totalRereads = 0;
                float sumRating = 0.0f;
                int   ratedCount = 0;
                string mostWatchedTitle = "N/A", mostReadTitle = "N/A";
                int   mostWatchedEps = -1, mostReadChs = -1;
                string highestRatedTitle = "N/A";
                int   highestRating = 0;

                for (int i = 0; i < (int)currentLibrary.size(); i++) {
                    const MediaRecord& m = currentLibrary[i];

                    if (m.type == "Anime") { totalAnime++; epsWatched += m.currentProgress; }
                    else                   { totalManga++; chsRead    += m.currentProgress; }

                    if (m.status == "Completed") completedCnt++;
                    if (m.status == "Dropped")   droppedCnt++;
                    if (m.status == "Watching")  watchingCnt++;
                    if (m.status == "Reading")   readingCnt++;

                    totalRereads += m.rereadCount;

                    sumRating += m.rating;
                    ratedCount++;

                    if (m.type == "Anime" && m.currentProgress > mostWatchedEps) {
                        mostWatchedEps   = m.currentProgress;
                        mostWatchedTitle = m.title;
                    }
                    if (m.type == "Manga" && m.currentProgress > mostReadChs) {
                        mostReadChs   = m.currentProgress;
                        mostReadTitle = m.title;
                    }
                    if (m.rating > highestRating) {
                        highestRating      = m.rating;
                        highestRatedTitle  = m.title;
                    }
                }

                float avgRating      = (ratedCount > 0) ? sumRating / ratedCount : 0.0f;
                float completionRate = (currentLibrary.size() > 0)
                    ? (completedCnt * 100.0f / currentLibrary.size()) : 0.0f;

                ImGuiTableFlags statFlags = ImGuiTableFlags_Borders
                                          | ImGuiTableFlags_RowBg
                                          | ImGuiTableFlags_SizingStretchProp;

                // -- SECTION 1: Overall Vault Status --
                ImGui::SetWindowFontScale(1.4f);
                ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.6f, 1.0f), "Overall Vault Status");
                ImGui::SetWindowFontScale(1.4f);
                ImGui::Spacing();

                if (ImGui::BeginTable("StatsVault", 2, statFlags)) {
                    ImGui::TableSetupColumn("Statistic", ImGuiTableColumnFlags_WidthStretch, 2.0f);
                    ImGui::TableSetupColumn("Value",     ImGuiTableColumnFlags_WidthStretch, 1.0f);
                    ImGui::TableHeadersRow();

                    auto statRow = [](const char* label, const char* value) {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0); ImGui::Text("%s", label);
                        ImGui::TableSetColumnIndex(1); ImGui::Text("%s", value);
                    };

                    char buf[64];
                    snprintf(buf, sizeof(buf), "%d", totalAnime);       statRow("Total Anime Tracked",   buf);
                    snprintf(buf, sizeof(buf), "%d", totalManga);       statRow("Total Manga Tracked",   buf);
                    snprintf(buf, sizeof(buf), "%d", completedCnt);     statRow("Completed Titles",      buf);
                    snprintf(buf, sizeof(buf), "%d", droppedCnt);       statRow("Dropped Titles",        buf);
                    snprintf(buf, sizeof(buf), "%d", watchingCnt);      statRow("Currently Watching",    buf);
                    snprintf(buf, sizeof(buf), "%d", readingCnt);       statRow("Currently Reading",     buf);
                    snprintf(buf, sizeof(buf), "%.1f%%", completionRate); statRow("Completion Rate",     buf);
                    ImGui::EndTable();
                }

                ImGui::Spacing(); ImGui::Spacing();

                // -- SECTION 2: Progress Metrics --
                ImGui::SetWindowFontScale(1.4f);
                ImGui::TextColored(ImVec4(0.35f, 0.65f, 1.0f, 1.0f), "Progress Metrics");
                ImGui::SetWindowFontScale(1.4f);
                ImGui::Spacing();

                if (ImGui::BeginTable("StatsProgress", 2, statFlags)) {
                    ImGui::TableSetupColumn("Metric", ImGuiTableColumnFlags_WidthStretch, 2.0f);
                    ImGui::TableSetupColumn("Value",  ImGuiTableColumnFlags_WidthStretch, 1.0f);
                    ImGui::TableHeadersRow();

                    auto statRow = [](const char* label, const char* value) {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0); ImGui::Text("%s", label);
                        ImGui::TableSetColumnIndex(1); ImGui::Text("%s", value);
                    };

                    char buf[64];
                    snprintf(buf, sizeof(buf), "%d", epsWatched);          statRow("Total Episodes Watched",     buf);
                    snprintf(buf, sizeof(buf), "%d", chsRead);             statRow("Total Chapters Read",        buf);
                    snprintf(buf, sizeof(buf), "%d", totalRereads);        statRow("Total Rewatches / Rereads",  buf);
                    snprintf(buf, sizeof(buf), "%.2f / 5.00", avgRating);  statRow("Average Rating",             buf);
                    ImGui::EndTable();
                }

                ImGui::Spacing(); ImGui::Spacing();

                // -- SECTION 3: Insights --
                // Notable records - most-watched, most-read, highest-rated
                ImGui::SetWindowFontScale(1.4f);
                ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.3f, 1.0f), "Insights");
                ImGui::SetWindowFontScale(1.4f);
                ImGui::Spacing();

                if (ImGui::BeginTable("StatsInsights", 2, statFlags)) {
                    ImGui::TableSetupColumn("Insight", ImGuiTableColumnFlags_WidthStretch, 2.0f);
                    ImGui::TableSetupColumn("Value",   ImGuiTableColumnFlags_WidthStretch, 2.5f);
                    ImGui::TableHeadersRow();

                    auto insightRow = [](const char* label, const char* value) {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0); ImGui::Text("%s", label);
                        ImGui::TableSetColumnIndex(1);
                        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.6f, 1.0f), "%s", value);
                    };

                    char buf[128];

                    snprintf(buf, sizeof(buf), "%.1f%%", completionRate);
                    insightRow("Completion Rate", buf);

                    insightRow("Most Watched Anime", mostWatchedTitle.c_str());
                    snprintf(buf, sizeof(buf), "%d episodes", mostWatchedEps < 0 ? 0 : mostWatchedEps);
                    insightRow("  Episodes Watched", buf);

                    insightRow("Most Read Manga", mostReadTitle.c_str());
                    snprintf(buf, sizeof(buf), "%d chapters", mostReadChs < 0 ? 0 : mostReadChs);
                    insightRow("  Chapters Read", buf);

                    insightRow("Highest Rated Title", highestRatedTitle.c_str());
                    snprintf(buf, sizeof(buf), "%d / 5", highestRating > 0 ? highestRating : 0);
                    insightRow("  Rating", buf);

                    ImGui::EndTable();
                }
            }

            // =================================================================
            //  TAB 4: ACTIVITY LOG
            //  Shows all logged events newest-first.
            //  Groups entries by date with a visible divider label between groups
            //  instead of alternating background colors.
            // =================================================================
            else if (currentTab == ACTIVITY_LOG) {
                ImGui::Text("Activity Log");
                ImGui::Separator(); ImGui::Spacing();

                if (activityLogs.empty()) {
                    ImGui::TextColored(ImVec4(0.55f, 0.55f, 0.55f, 1.0f),
                        "No activity recorded yet.");
                } else {
                    ImGui::BeginChild("LogScroll", ImVec2(0, 0), true);

                    string lastDate = "";

                    // Iterate newest-first (reverse order)
                    for (int i = (int)activityLogs.size() - 1; i >= 0; i--) {
                        const string& log = activityLogs[i];

                        // Extract the date from the "[YYYY-MM-DD]" prefix
                        string date = "";
                        if (log.size() > 12 && log[0] == '[')
                            date = log.substr(1, 10);

                        // When the date changes, insert a visual date-divider
                        if (date != lastDate) {
                            lastDate = date;
                            if (i < (int)activityLogs.size() - 1)
                                ImGui::Spacing();
                            string divider = "---- " + (date.empty() ? "Unknown Date" : date) + " ----";
                            ImGui::TextColored(ImVec4(0.5f, 0.75f, 1.0f, 0.9f), "%s", divider.c_str());
                            ImGui::Spacing();
                        }

                        // Strip the date prefix from the log entry for cleaner display
                        string displayLog = log;
                        if (log.size() > 13 && log[0] == '[')
                            displayLog = log.substr(13); // skip "[YYYY-MM-DD] "

                        ImGui::TextWrapped("%s", displayLog.c_str());
                        ImGui::Separator();
                    }

                    ImGui::EndChild();
                }
            }

            ImGui::End(); // End main content area
        }

        // -- Finalize and render the frame --
        ImGui::Render();
        glClearColor(0.10f, 0.10f, 0.12f, 1.0f); // Flat dark background color
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window); // Show the finished frame on screen
    }

    // -- Cleanup - release all resources before exiting --
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
