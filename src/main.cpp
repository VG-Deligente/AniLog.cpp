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

using namespace std;

// =============================================================================
//  ANILOG - Complete Inventory & Customer Ordering System
//  COMP 003 | Final Laboratory Project
// =============================================================================

enum Screen { LOGIN, SIGNUP, DASHBOARD };
// Added STATISTICS to the DashboardTab enum
enum DashboardTab { LIBRARY, ADD_MEDIA, EDIT_MEDIA, ACTIVITY_LOG, STATISTICS };

// --- DATA STRUCTURES ---

struct UserRecord {
    string username;
    string password;
};

struct MediaRecord {
    string title;
    string type;           // Anime or Manga
    int currentProgress;   // Watched/Read (Deducted Stock)
    int totalProgress;     // Total Episodes/Chapters (Total Stock)
    int rating;            // 1 to 5
    string status;         // "Watching", "Reading", "Completed", "Dropped"
    string dateStarted;    
    string dateFinished;   
    int rereadCount;       
};

// --- GLOBAL VARIABLES ---

vector<UserRecord> userDatabase;
vector<MediaRecord> currentLibrary;
vector<string> activityLogs;

string loggedInUser = "";
string authMessage = "";
bool isAuthError = false;

Screen currentScreen = LOGIN;
DashboardTab currentTab = LIBRARY;

// Input buffers for Add/Edit forms
char inputTitle[128] = "";
int inputTypeIndex = 0;     // 0 = Anime, 1 = Manga
int inputCurrent = 0;
int inputTotal = 12;
int inputRating = 1;
int inputStatusIndex = 0;   // 0 = Active (Watching/Reading), 1 = Completed, 2 = Dropped
int editingIndex = -1;      

// Filter and Search buffers
char searchBuffer[128] = "";
int currentFilterIndex = 0; // 0 = All, 1 = Anime, 2 = Manga

// Helper arrays to map UI selections to string data
const char* typeOptions[] = { "Anime", "Manga" };
const char* animeStatusOptions[] = { "Watching", "Completed", "Dropped" };
const char* mangaStatusOptions[] = { "Reading", "Completed", "Dropped" };
const char* filterOptions[] = { "All Media", "Anime Only", "Manga Only" };

// --- UTILITY FUNCTIONS ---

string getCurrentDate() {
    time_t now = time(0);
    tm* ltm = localtime(&now);
    char dateStr[11];
    snprintf(dateStr, sizeof(dateStr), "%04d-%02d-%02d", 1900 + ltm->tm_year, 1 + ltm->tm_mon, ltm->tm_mday);
    return string(dateStr);
}

void logActivity(string action) {
    string entry = "[" + getCurrentDate() + "] " + action;
    activityLogs.push_back(entry);

    ofstream file(loggedInUser + "_logs.txt", ios::app);
    if (file.is_open()) {
        file << entry << "\n";
        file.close();
    }
}

// --- SORTING ALGORITHMS ---

bool sortTitleAsc(const MediaRecord& a, const MediaRecord& b) { return a.title < b.title; }
bool sortTitleDesc(const MediaRecord& a, const MediaRecord& b) { return a.title > b.title; }
bool sortRatingDesc(const MediaRecord& a, const MediaRecord& b) { return a.rating > b.rating; }
bool sortProgressDesc(const MediaRecord& a, const MediaRecord& b) { 
    float pctA = (a.totalProgress > 0) ? (float)a.currentProgress / a.totalProgress : 0;
    float pctB = (b.totalProgress > 0) ? (float)b.currentProgress / b.totalProgress : 0;
    return pctA > pctB; 
}

// --- FILE HANDLING ---

void loadUsers() {
    ifstream file("users.txt");
    if (!file.is_open()) return;
    UserRecord temp;
    while (getline(file, temp.username, ',') && getline(file, temp.password)) {
        userDatabase.push_back(temp);
    }
    file.close();
}

void saveUser(UserRecord u) {
    ofstream file("users.txt", ios::app);
    if (file.is_open()) {
        file << u.username << "," << u.password << "\n";
        file.close();
    }
}

void loadLibrary() {
    currentLibrary.clear();
    activityLogs.clear();

    ifstream file(loggedInUser + "_library.txt");
    if (file.is_open()) {
        string line, val;
        while (getline(file, line)) {
            stringstream ss(line);
            MediaRecord m;
            getline(ss, m.title, ',');
            getline(ss, m.type, ',');
            
            getline(ss, val, ','); m.currentProgress = stoi(val);
            getline(ss, val, ','); m.totalProgress = stoi(val);
            getline(ss, val, ','); m.rating = stoi(val);
            
            getline(ss, m.status, ',');
            getline(ss, m.dateStarted, ',');
            getline(ss, m.dateFinished, ',');
            
            getline(ss, val); m.rereadCount = stoi(val);
            
            currentLibrary.push_back(m);
        }
        file.close();
    }

    ifstream logFile(loggedInUser + "_logs.txt");
    if (logFile.is_open()) {
        string logLine;
        while (getline(logFile, logLine)) {
            activityLogs.push_back(logLine);
        }
        logFile.close();
    }
}

void saveLibrary() {
    ofstream file(loggedInUser + "_library.txt");
    if (file.is_open()) {
        for (int i = 0; i < currentLibrary.size(); i++) {
            MediaRecord m = currentLibrary[i];
            file << m.title << "," << m.type << "," 
                 << m.currentProgress << "," << m.totalProgress << "," 
                 << m.rating << "," << m.status << "," 
                 << m.dateStarted << "," << m.dateFinished << "," 
                 << m.rereadCount << "\n";
        }
        file.close();
    }
}

// --- AUTHENTICATION LOGIC ---

void clearMessage() { authMessage = ""; isAuthError = false; }

bool registerUser(string username, string password) {
    if (username == "" || password == "") {
        authMessage = "All fields are required."; isAuthError = true; return false;
    }
    if (password.length() < 6) {
        authMessage = "Password must be at least 6 characters."; isAuthError = true; return false;
    }
    for (int i = 0; i < userDatabase.size(); i++) {
        if (userDatabase[i].username == username) {
            authMessage = "Username already taken."; isAuthError = true; return false;
        }
    }
    UserRecord newUser; newUser.username = username; newUser.password = password;
    userDatabase.push_back(newUser);
    saveUser(newUser);
    authMessage = "Account created! You can now log in."; isAuthError = false;
    return true;
}

bool loginUser(string username, string password) {
    if (username == "" || password == "") {
        authMessage = "Please fill in both fields."; isAuthError = true; return false;
    }
    for (int i = 0; i < userDatabase.size(); i++) {
        if (userDatabase[i].username == username && userDatabase[i].password == password) {
            loggedInUser = username;
            loadLibrary(); 
            logActivity("User Login: " + loggedInUser + " signed in.");
            clearMessage();
            return true;
        }
    }
    authMessage = "Incorrect username or password."; isAuthError = true;
    return false;
}

void resetForm() {
    inputTitle[0] = '\0';
    inputTypeIndex = 0;
    inputCurrent = 0;
    inputTotal = 12;
    inputRating = 3;
    inputStatusIndex = 0;
    editingIndex = -1;
}

// =============================================================================
//  MAIN PROGRAM / GUI RENDER LOOP
// =============================================================================

int main() {
    if (!glfwInit()) return 1;
    GLFWwindow* window = glfwCreateWindow(1280, 720, "AniLog - Inventory System", NULL, NULL);
    glfwMakeContextCurrent(window);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 8.0f;
    style.FrameRounding = 6.0f;
    style.GrabRounding = 6.0f;
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.08f, 0.12f, 0.0f); //
    style.Colors[ImGuiCol_Button] = ImVec4(0.18f, 0.35f, 0.58f, 1.0f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.24f, 0.45f, 0.75f, 1.0f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.12f, 0.25f, 0.45f, 1.0f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.18f, 0.35f, 0.58f, 0.8f);

    loadUsers();

    char usernameInput[64] = "";
    char passwordInput[64] = "";
    bool showPassword = false;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame(); ImGui_ImplGlfw_NewFrame(); ImGui::NewFrame();
        ImGuiIO& io = ImGui::GetIO();

        // ---------------------------------------------------------------------
        // AUTHENTICATION SCREENS
        // ---------------------------------------------------------------------
        if (currentScreen == LOGIN || currentScreen == SIGNUP) {
            float winW = 600.0f, winH = 600.0f, elemW = 480.0f;
            float offsetX = (winW - elemW) * 0.5f;

            ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(winW, winH));
            
            ImGui::Begin(currentScreen == LOGIN ? "Login" : "Signup", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
            
            ImGui::Spacing(); ImGui::SetWindowFontScale(3.0f);
            float titleW = ImGui::CalcTextSize("ANILOG").x;
            ImGui::SetCursorPosX((winW - titleW) * 0.5f);
            ImGui::TextColored(ImVec4(0.35f, 0.65f, 1.0f, 1.0f), "ANILOG");
            
            ImGui::SetWindowFontScale(1.5f); 
            ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
            ImGui::Dummy(ImVec2(0, 50));

            ImGui::SetCursorPosX(offsetX); ImGui::Text(currentScreen == LOGIN ? "Username" : "Choose Username");
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 12)); 
            ImGui::SetCursorPosX(offsetX); ImGui::SetNextItemWidth(elemW);
            ImGui::InputText("##user", usernameInput, IM_ARRAYSIZE(usernameInput));
            ImGui::PopStyleVar();
            ImGui::Spacing();

            ImGui::SetCursorPosX(offsetX); ImGui::Text(currentScreen == LOGIN ? "Password" : "Create Password"); 
            ImGui::SetCursorPosX(offsetX); 
            float toggleBtnW = 80.0f;
            float inputH = ImGui::GetFrameHeight(); 

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 12));
            ImGui::SetNextItemWidth(elemW - toggleBtnW - 8.0f);
            ImGuiInputTextFlags passFlags = showPassword ? ImGuiInputTextFlags_None : ImGuiInputTextFlags_Password;
            ImGui::InputText("##pass", passwordInput, IM_ARRAYSIZE(passwordInput), passFlags);
            ImGui::PopStyleVar();
            ImGui::SameLine();
            if (ImGui::Button(showPassword ? "Hide" : "Show", ImVec2(toggleBtnW, inputH))) showPassword = !showPassword;

            if (authMessage != "") {
                ImGui::Spacing(); ImGui::SetCursorPosX(offsetX);
                if (isAuthError) ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", authMessage.c_str());
                else ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.6f, 1.0f), "%s", authMessage.c_str());
            }

            ImGui::Spacing(); ImGui::Spacing(); ImGui::SetCursorPosX(offsetX);
            if (currentScreen == LOGIN) {
                if (ImGui::Button("Log In", ImVec2(elemW, 50))) {
                    if (loginUser(usernameInput, passwordInput)) {
                        currentScreen = DASHBOARD; currentTab = LIBRARY; resetForm();
                    }
                }
                ImGui::Spacing(); ImGui::SetCursorPosX(offsetX);
                if (ImGui::Button("Need an account? Sign Up", ImVec2(elemW, 40))) {
                    currentScreen = SIGNUP; clearMessage(); usernameInput[0]='\0'; passwordInput[0]='\0';
                }
            } else {
                if (ImGui::Button("Create Account", ImVec2(elemW, 50))) {
                    if (registerUser(usernameInput, passwordInput)) {
                        currentScreen = LOGIN; usernameInput[0]='\0'; passwordInput[0]='\0';
                    }
                }
                ImGui::Spacing(); ImGui::SetCursorPosX(offsetX);
                if (ImGui::Button("Back to Log In", ImVec2(elemW, 40))) {
                    currentScreen = LOGIN; clearMessage(); usernameInput[0]='\0'; passwordInput[0]='\0';
                }
            }
            ImGui::End();
        }

        // ---------------------------------------------------------------------
        // DASHBOARD (INVENTORY SYSTEM)
        // ---------------------------------------------------------------------
        else if (currentScreen == DASHBOARD) {
            ImDrawList* bg = ImGui::GetBackgroundDrawList();

            bg->AddRectFilledMultiColor(
                ImVec2(0, 0),
                ImVec2(io.DisplaySize.x, io.DisplaySize.y),
                IM_COL32(20,  10,  60,  255),  // top-left     (deep purple)
                IM_COL32(10,  30,  80,  255),  // top-right    (dark blue)
                IM_COL32(10,  60,  80,  255),  // bottom-right (teal)
                IM_COL32(40,  10,  60,  255)   // bottom-left  (violet)
            );

            // --- SIDEBAR MENU ---
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(250, io.DisplaySize.y));
            ImGui::Begin("Sidebar", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
            ImGui::SetWindowFontScale(1.5f);
            
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.35f, 0.65f, 1.0f, 1.0f), "ANILOG");
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "User: %s", loggedInUser.c_str());
            ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

            if (ImGui::Button("AniDex", ImVec2(230, 45))) currentTab = LIBRARY;
            ImGui::Spacing();
            if (ImGui::Button("Add Record", ImVec2(230, 45))) { currentTab = ADD_MEDIA; resetForm(); }
            ImGui::Spacing();
            if (ImGui::Button("Statistics", ImVec2(230, 45))) currentTab = STATISTICS;
            ImGui::Spacing();
            if (ImGui::Button("Activity Log", ImVec2(230, 45))) currentTab = ACTIVITY_LOG;
            
           ImGui::SetCursorPosY(io.DisplaySize.y - 70);
            ImGui::Separator(); ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.3f, 0.3f, 1.0f));
            if (ImGui::Button("Logout", ImVec2(230, 40))) {
                ImGui::OpenPopup("Logout Confirmation");
            }
            ImGui::PopStyleColor(2);

            // Confirmation modal
            ImVec2 center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            if (ImGui::BeginPopupModal("Logout Confirmation", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("Are you sure you want to logout?");
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.3f, 0.3f, 1.0f));
                if (ImGui::Button("Yes, Logout", ImVec2(120, 35))) {
                    logActivity("User Logout: " + loggedInUser + " signed out.");
                    currentScreen = LOGIN; loggedInUser = ""; usernameInput[0]='\0'; passwordInput[0]='\0';
                    ImGui::CloseCurrentPopup();
                }
                ImGui::PopStyleColor(2);

                ImGui::SameLine();
                ImGui::Spacing(); ImGui::SameLine();

                if (ImGui::Button("Cancel", ImVec2(120, 35))) {
                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
            }

            ImGui::End();

            // --- MAIN CONTENT AREA ---
            ImGui::SetNextWindowPos(ImVec2(250, 0));
            ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x - 250, io.DisplaySize.y));
            ImGui::Begin("Content", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
            ImGui::SetWindowFontScale(1.5f); // originally 1.3f, increased for better visibility

            // ==========================================
            // TAB 1: LIBRARY
            // ==========================================
            if (currentTab == LIBRARY) {
                ImGui::Text("My Media Vault (Inventory)");
                ImGui::Separator(); ImGui::Spacing();
                
                ImGui::SetNextItemWidth(300);
                ImGui::InputTextWithHint("##search", "Search Title...", searchBuffer, IM_ARRAYSIZE(searchBuffer));
                string searchStr = searchBuffer;
                
                ImGui::SameLine();
                ImGui::SetNextItemWidth(150);
                ImGui::Combo("Filter", &currentFilterIndex, filterOptions, IM_ARRAYSIZE(filterOptions));

                ImGui::SameLine();
                ImGui::Text("  Sort:");
                ImGui::SameLine();
                if (ImGui::Button("A-Z")) std::sort(currentLibrary.begin(), currentLibrary.end(), sortTitleAsc);
                ImGui::SameLine();
                if (ImGui::Button("Z-A")) std::sort(currentLibrary.begin(), currentLibrary.end(), sortTitleDesc);
                ImGui::SameLine();
                if (ImGui::Button("Rating")) std::sort(currentLibrary.begin(), currentLibrary.end(), sortRatingDesc);
                ImGui::SameLine();
                if (ImGui::Button("Progress")) std::sort(currentLibrary.begin(), currentLibrary.end(), sortProgressDesc);

                ImGui::Spacing();
                
                if (ImGui::BeginTable("LibraryTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
                    ImGui::TableSetupColumn("Title", ImGuiTableColumnFlags_WidthStretch, 2.5f);
                    ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthStretch, 1.0f);
                    ImGui::TableSetupColumn("Progress", ImGuiTableColumnFlags_WidthStretch, 1.2f);
                    ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthStretch, 1.5f);
                    ImGui::TableSetupColumn("Rating", ImGuiTableColumnFlags_WidthStretch, 0.8f);
                    ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthStretch, 2.5f);
                    ImGui::TableHeadersRow();

                    for (int i = 0; i < currentLibrary.size(); i++) {
                        
                        if (currentFilterIndex == 1 && currentLibrary[i].type != "Anime") continue;
                        if (currentFilterIndex == 2 && currentLibrary[i].type != "Manga") continue;
                        if (searchStr != "" && currentLibrary[i].title.find(searchStr) == string::npos) continue;

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0); ImGui::TextWrapped("%s", currentLibrary[i].title.c_str());
                        ImGui::TableSetColumnIndex(1); ImGui::Text("%s", currentLibrary[i].type.c_str());
                        ImGui::TableSetColumnIndex(2); ImGui::Text("%d / %d", currentLibrary[i].currentProgress, currentLibrary[i].totalProgress);
                        ImGui::TableSetColumnIndex(3); ImGui::Text("%s", currentLibrary[i].status.c_str());
                        ImGui::TableSetColumnIndex(4); ImGui::Text("%d/5", currentLibrary[i].rating);
                        ImGui::TableSetColumnIndex(5);
                        
                        ImGui::PushID(i);
                        if (currentLibrary[i].currentProgress < currentLibrary[i].totalProgress) {
                            // Dynamic button label based on type
                            string btnLabel = (currentLibrary[i].type == "Anime") ? "+1 Episode" : "+1 Chapter";
                            
                            if (ImGui::Button(btnLabel.c_str(), ImVec2(120, 0))) {
                                currentLibrary[i].currentProgress++;
                                logActivity("Progress Updated: Advanced 1 unit in [" + currentLibrary[i].title + "]");
                                
                                // WILL BE MOVED TO ANOTHER TABLE IN THE VAULT TAB
                                if (currentLibrary[i].currentProgress == currentLibrary[i].totalProgress) {
                                    currentLibrary[i].status = "Completed";
                                    currentLibrary[i].dateFinished = getCurrentDate();
                                    logActivity("Status Changed: [" + currentLibrary[i].title + "] marked as Completed.");
                                }
                                saveLibrary();
                            }
                        }
                        // Edit Logic in Vault Tab
                        ImGui::SameLine();
                        if (ImGui::Button("Edit")) {
                            editingIndex = i;
                            snprintf(inputTitle, sizeof(inputTitle), "%s", currentLibrary[i].title.c_str());
                            inputTypeIndex = (currentLibrary[i].type == "Anime") ? 0 : 1;
                            inputCurrent = currentLibrary[i].currentProgress;
                            inputTotal = currentLibrary[i].totalProgress;
                            inputRating = currentLibrary[i].rating;
                            
                            if (currentLibrary[i].status == "Completed") inputStatusIndex = 1;
                            else if (currentLibrary[i].status == "Dropped") inputStatusIndex = 2;
                            else inputStatusIndex = 0; 

                            currentTab = EDIT_MEDIA;
                        }
                        
                        // Delete Logic in Vault Tab
                        ImGui::SameLine();
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
                        if (ImGui::Button("Del")) {
                            ImGui::OpenPopup("Delete Confirmation");
                        }
                        ImGui::PopStyleColor(2);

                        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
                        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
                        if (ImGui::BeginPopupModal("Delete Confirmation", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                            ImGui::Text("Delete \"%s\"?", currentLibrary[i].title.c_str());
                            ImGui::Text("This action cannot be undone.");
                            ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

                            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
                            if (ImGui::Button("Yes, Delete", ImVec2(120, 35))) {
                                logActivity("Record Deleted: [" + currentLibrary[i].title + "] removed from the library.");
                                currentLibrary.erase(currentLibrary.begin() + i);
                                saveLibrary();
                                ImGui::CloseCurrentPopup();
                            }
                            ImGui::PopStyleColor(2);

                            ImGui::SameLine(); ImGui::Spacing(); ImGui::SameLine();
                            if (ImGui::Button("Cancel", ImVec2(120, 35))) {
                                ImGui::CloseCurrentPopup();
                            }
                            ImGui::EndPopup();
                        }
                        ImGui::PopID();
                    }
                    ImGui::EndTable();
                }
                    ImGui::Spacing();
                    ImGui::Spacing();
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.6f, 1.0f), "Completed Media Records");
                    ImGui::SetWindowFontScale(1.3f);
                    ImGui::Spacing();
                    if (ImGui::BeginTable("Completed Content", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
                        ImGui::TableSetupColumn("Title", ImGuiTableColumnFlags_WidthStretch, 2.5f);
                        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthStretch, 1.0f);
                        ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthStretch, 1.5f);
                        ImGui::TableSetupColumn("Rating", ImGuiTableColumnFlags_WidthStretch, 0.8f);
                        ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthStretch, 2.5f);
                        ImGui::TableHeadersRow();
                        for (int i = 0; i < currentLibrary.size(); i++) {
                            if (currentLibrary[i].status != "Completed") continue;
                            if (currentFilterIndex == 1 && currentLibrary[i].type != "Anime") continue;
                            if (currentFilterIndex == 2 && currentLibrary[i].type != "Manga") continue;
                            if (searchStr != "" && currentLibrary[i].title.find(searchStr) == string::npos) continue;
                           
                            ImGui::PushID(100 + i);
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0); ImGui::TextWrapped("%s", currentLibrary[i].title.c_str());
                            ImGui::TableSetColumnIndex(1); ImGui::Text("%s", currentLibrary[i].type.c_str());
                            ImGui::TableSetColumnIndex(2); ImGui::Text("%s", currentLibrary[i].status.c_str());
                            ImGui::TableSetColumnIndex(3); ImGui::Text("%d/5", currentLibrary[i].rating);
                            ImGui::TableSetColumnIndex(4);
                            

                            string rBtnLabel = (currentLibrary[i].type == "Anime") ? "Rewatch" : "Reread";
                                if (ImGui::Button(rBtnLabel.c_str(), ImVec2(120, 0))) {
                                    currentLibrary[i].rereadCount++;
                                    currentLibrary[i].currentProgress = 0; 
                                    currentLibrary[i].status = (currentLibrary[i].type == "Anime") ? "Watching" : "Reading";
                                    logActivity("Record Reset: [" + currentLibrary[i].title + "] restarted for rewatch/reread.");
                                    saveLibrary();
                                }
                            
                            ImGui::PopID();
                        
                        }
                    ImGui::EndTable();
                }
            }
            
            
            // ==========================================
            // TAB 2: ADD / EDIT MEDIA 
            // ==========================================
            else if (currentTab == ADD_MEDIA || currentTab == EDIT_MEDIA) {
                ImGui::Text(currentTab == ADD_MEDIA ? "Add New Inventory Record" : "Edit Inventory Record");
                ImGui::Separator(); ImGui::Spacing();

                ImGui::SetNextItemWidth(400);
                ImGui::InputText("Title", inputTitle, IM_ARRAYSIZE(inputTitle));
                
                ImGui::Spacing(); ImGui::Text("Category Type:"); ImGui::SameLine(150);
                ImGui::RadioButton("Anime", &inputTypeIndex, 0); ImGui::SameLine();
                ImGui::RadioButton("Manga", &inputTypeIndex, 1);
                ImGui::Spacing();

                ImGui::SetNextItemWidth(150);
                // Dynamic label for progress input
                ImGui::InputInt(inputTypeIndex == 0 ? "Current Progress (Episodes)" : "Current Progress (Chapters)", &inputCurrent);
                ImGui::SetNextItemWidth(150);
                ImGui::InputInt(inputTypeIndex == 0 ? "Total Episodes" : "Total Chapters", &inputTotal);
                
                if (inputCurrent < 0) inputCurrent = 0;
                if (inputTotal < 1) inputTotal = 1;
                if (inputCurrent > inputTotal) inputCurrent = inputTotal; 

                ImGui::SetNextItemWidth(150);
                ImGui::SliderInt("Rating", &inputRating, 1, 5); 

                ImGui::SetNextItemWidth(200);
                // Conditionally assign the correct status options based on the chosen type
                const char** currentStatusOptions = (inputTypeIndex == 0) ? animeStatusOptions : mangaStatusOptions;
                ImGui::Combo("Status", &inputStatusIndex, currentStatusOptions, 3);

                ImGui::Spacing(); ImGui::Spacing();
                
                if (ImGui::Button("Save Record", ImVec2(200, 50))) {
                    if (string(inputTitle) != "") {
                        if (currentTab == ADD_MEDIA) {
                            MediaRecord newMedia;
                            newMedia.title = inputTitle;
                            newMedia.type = typeOptions[inputTypeIndex];
                            newMedia.currentProgress = inputCurrent;
                            newMedia.totalProgress = inputTotal;
                            newMedia.rating = inputRating;
                            newMedia.status = currentStatusOptions[inputStatusIndex];
                            newMedia.dateStarted = getCurrentDate();
                            newMedia.dateFinished = (inputCurrent == inputTotal) ? getCurrentDate() : "";
                            newMedia.rereadCount = 0;
                            
                            currentLibrary.push_back(newMedia);
                            logActivity("Record Added: [" + newMedia.title + "] added to the library.");
                        } else {
                            currentLibrary[editingIndex].title = inputTitle;
                            currentLibrary[editingIndex].type = typeOptions[inputTypeIndex];
                            currentLibrary[editingIndex].currentProgress = inputCurrent;
                            currentLibrary[editingIndex].totalProgress = inputTotal;
                            currentLibrary[editingIndex].rating = inputRating;
                            currentLibrary[editingIndex].status = currentStatusOptions[inputStatusIndex];
                            
                            if (inputCurrent == inputTotal && currentLibrary[editingIndex].dateFinished == "") {
                                currentLibrary[editingIndex].dateFinished = getCurrentDate();
                            }

                            logActivity("Record Updated: [" + string(inputTitle) + "] details have been modified.");
                        }
                        saveLibrary();
                        currentTab = LIBRARY;
                        resetForm();
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel", ImVec2(100, 50))) {
                    currentTab = LIBRARY;
                    resetForm();
                }
            }

           // ==========================================
            // TAB 3: STATISTICS
            // ==========================================
            else if (currentTab == STATISTICS) {
                ImGui::Text("User Statistics & Insights");
                ImGui::Separator(); ImGui::Spacing();

                int totalAnime = 0, totalManga = 0;
                int epsWatched = 0, chsRead = 0;
                int completedCount = 0, droppedCount = 0, watchingCount = 0, readingCount = 0;
                int totalRereads = 0;

                for (size_t i = 0; i < currentLibrary.size(); i++) {
                    if (currentLibrary[i].type == "Anime") { totalAnime++; epsWatched += currentLibrary[i].currentProgress; }
                    else if (currentLibrary[i].type == "Manga") { totalManga++; chsRead += currentLibrary[i].currentProgress; }
                    if (currentLibrary[i].status == "Completed") completedCount++;
                    if (currentLibrary[i].status == "Dropped") droppedCount++;
                    if (currentLibrary[i].status == "Watching") watchingCount++;
                    if (currentLibrary[i].status == "Reading") readingCount++;
                    totalRereads += currentLibrary[i].rereadCount;
                }

                float avgRating = 0.0f;
                int ratedCount = 0;
                string mostWatchedTitle = "N/A", mostReadTitle = "N/A";
                int mostWatchedEps = -1, mostReadChs = -1;

                for (size_t i = 0; i < currentLibrary.size(); i++) {
                    avgRating += currentLibrary[i].rating;
                    ratedCount++;

                    if (currentLibrary[i].type == "Anime" && currentLibrary[i].currentProgress > mostWatchedEps) {
                        mostWatchedEps = currentLibrary[i].currentProgress;
                        mostWatchedTitle = currentLibrary[i].title;
                    }
                    if (currentLibrary[i].type == "Manga" && currentLibrary[i].currentProgress > mostReadChs) {
                        mostReadChs = currentLibrary[i].currentProgress;
                        mostReadTitle = currentLibrary[i].title;
                    }
                }
                if (ratedCount > 0) avgRating /= ratedCount;

                float completionRate = (currentLibrary.size() > 0)
                    ? (completedCount * 100.0f / currentLibrary.size()) : 0.0f;

                // ── SECTION 1: Overall Vault Status ──
                ImGui::SetWindowFontScale(1.4f);
                ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.6f, 1.0f), "Overall Vault Status");
                ImGui::SetWindowFontScale(1.3f);
                ImGui::Spacing();

                if (ImGui::BeginTable("StatsVault", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
                    ImGui::TableSetupColumn("Statistic", ImGuiTableColumnFlags_WidthStretch, 2.0f);
                    ImGui::TableSetupColumn("Value",     ImGuiTableColumnFlags_WidthStretch, 1.0f);
                    ImGui::TableHeadersRow();

                    auto addRow = [](const char* label, const char* value) {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0); ImGui::Text("%s", label);
                        ImGui::TableSetColumnIndex(1); ImGui::Text("%s", value);
                    };

                    char buf[64];
                    snprintf(buf, sizeof(buf), "%d", totalAnime);      addRow("Total Anime Tracked", buf);
                    snprintf(buf, sizeof(buf), "%d", totalManga);      addRow("Total Manga Tracked", buf);
                    snprintf(buf, sizeof(buf), "%d", completedCount);  addRow("Completed Titles", buf);
                    snprintf(buf, sizeof(buf), "%d", droppedCount);    addRow("Dropped Titles", buf);
                    snprintf(buf, sizeof(buf), "%d", watchingCount);   addRow("Currently Watching", buf);
                    snprintf(buf, sizeof(buf), "%d", readingCount);    addRow("Currently Reading", buf);
                    snprintf(buf, sizeof(buf), "%.1f%%", completionRate); addRow("Completion Rate", buf);

                    ImGui::EndTable();
                }

                ImGui::Spacing(); ImGui::Spacing();

                // ── SECTION 2: Consumption Metrics ──
                ImGui::SetWindowFontScale(1.4f);
                ImGui::TextColored(ImVec4(0.35f, 0.65f, 1.0f, 1.0f), "Consumption Metrics");
                ImGui::SetWindowFontScale(1.3f);
                ImGui::Spacing();

                if (ImGui::BeginTable("StatsConsumption", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
                    ImGui::TableSetupColumn("Metric", ImGuiTableColumnFlags_WidthStretch, 2.0f);
                    ImGui::TableSetupColumn("Value",  ImGuiTableColumnFlags_WidthStretch, 1.0f);
                    ImGui::TableHeadersRow();

                    auto addRow = [](const char* label, const char* value) {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0); ImGui::Text("%s", label);
                        ImGui::TableSetColumnIndex(1); ImGui::Text("%s", value);
                    };

                    char buf[64];
                    snprintf(buf, sizeof(buf), "%d", epsWatched);   addRow("Total Episodes Watched", buf);
                    snprintf(buf, sizeof(buf), "%d", chsRead);      addRow("Total Chapters Read", buf);
                    snprintf(buf, sizeof(buf), "%d", totalRereads); addRow("Total Rewatches / Rereads", buf);
                    snprintf(buf, sizeof(buf), "%.2f / 5", avgRating); addRow("Average Rating", buf);
                    addRow("Most Watched Anime", mostWatchedTitle.c_str());
                    addRow("Most Read Manga",    mostReadTitle.c_str());

                    ImGui::EndTable();
                }

                ImGui::Spacing(); ImGui::Spacing();

                // ── SECTION 3: Insights ──
                ImGui::SetWindowFontScale(1.4f);
                ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.3f, 1.0f), "Insights");
                ImGui::SetWindowFontScale(1.3f);
                ImGui::Spacing();

                if (ImGui::BeginTable("StatsInsights", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
                    ImGui::TableSetupColumn("Insight",  ImGuiTableColumnFlags_WidthStretch, 2.0f);
                    ImGui::TableSetupColumn("Value",    ImGuiTableColumnFlags_WidthStretch, 2.5f);
                    ImGui::TableHeadersRow();

                    auto addRow = [](const char* label, const char* value) {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0); ImGui::Text("%s", label);
                        ImGui::TableSetColumnIndex(1); ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.6f, 1.0f), "%s", value);
                    };

                    char buf[64];

                    snprintf(buf, sizeof(buf), "%.2f / 5.00", avgRating);
                    addRow("Average Rating", buf);

                    snprintf(buf, sizeof(buf), "%.1f%%", completionRate);
                    addRow("Completion Rate", buf);

                    addRow("Most Watched Anime", mostWatchedTitle.c_str());

                    snprintf(buf, sizeof(buf), "%d eps", mostWatchedEps < 0 ? 0 : mostWatchedEps);
                    addRow("  Episodes Watched", buf);

                    addRow("Most Read Manga", mostReadTitle.c_str());

                    snprintf(buf, sizeof(buf), "%d chs", mostReadChs < 0 ? 0 : mostReadChs);
                    addRow("  Chapters Read", buf);

                    ImGui::EndTable();
                }
            }

            // ==========================================
            // TAB 4: ACTIVITY LOG
            // ==========================================
            else if (currentTab == ACTIVITY_LOG) {
                ImGui::Text("Activity Log - Record Management History");
                ImGui::Separator(); ImGui::Spacing();

                ImGui::BeginChild("LogScroll", ImVec2(0, 0), true);

                ImVec4 dateColors[] = {
                    ImVec4(0.18f, 0.35f, 0.58f, 0.4f),
                    ImVec4(0.35f, 0.18f, 0.58f, 0.4f)
                };

                string lastDate = "";
                int colorIndex = -1;

                for (int i = activityLogs.size() - 1; i >= 0; i--) {
                    string log = activityLogs[i];
                    string date = "";
                    if (log.size() > 12 && log[0] == '[')
                        date = log.substr(1, 10);

                    if (date != lastDate) { lastDate = date; colorIndex = (colorIndex + 1) % 2; }

                    ImVec2 rowMin = ImGui::GetCursorScreenPos();
                    ImVec2 rowMax = ImVec2(rowMin.x + ImGui::GetContentRegionAvail().x, rowMin.y + ImGui::GetTextLineHeightWithSpacing() * 2);
                    ImGui::GetWindowDrawList()->AddRectFilled(rowMin, rowMax, ImGui::ColorConvertFloat4ToU32(dateColors[colorIndex]), 4.0f);

                    ImGui::TextWrapped("%s", log.c_str());
                    ImGui::Separator();
                }

                ImGui::EndChild();
            }
            ImGui::End();
        }
        ImGui::Render();
        glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}