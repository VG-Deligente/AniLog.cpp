#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <string>
#include <fstream>
#include <vector>
#include <sstream>
#include <ctime>

using namespace std;

// =============================================================================
//  ANILOG - Complete Inventory & Customer Ordering System
//  COMP 003 | Final Laboratory Project
// =============================================================================

enum Screen { LOGIN, SIGNUP, DASHBOARD };
enum DashboardTab { LIBRARY, ADD_MEDIA, EDIT_MEDIA, ACTIVITY_LOG };

// --- DATA STRUCTURES ---
struct UserRecord {
    string username;
    string password;
};

// RUBRIC: Inventory Data Structure
struct MediaRecord {
    string title;
    string type;           // Anime or Manga
    int currentProgress;   // Watched/Read (Deducted Stock)
    int totalProgress;     // Total Episodes (Total Stock)
    int rating;            // 1 to 5
    string status;         // "Reading/Watching", "Completed", "Dropped"
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
char inputType[32] = "Anime";
int inputCurrent = 0;
int inputTotal = 12;
int inputRating = 0;
char inputStatus[32] = "Watching";
int editingIndex = -1; // Tracks which inventory item is being updated

// Search buffer
char searchBuffer[128] = "";

// --- UTILITY FUNCTIONS ---

// Gets today's date as a string (YYYY-MM-DD)
string getCurrentDate() {
    time_t now = time(0);
    tm* ltm = localtime(&now);
    char dateStr[11];
    snprintf(dateStr, sizeof(dateStr), "%04d-%02d-%02d", 1900 + ltm->tm_year, 1 + ltm->tm_mon, ltm->tm_mday);
    return string(dateStr);
}

// RUBRIC: Generate transaction summaries or order receipts
void logActivity(string action) {
    string entry = "[" + getCurrentDate() + "] " + action;
    activityLogs.push_back(entry);

    ofstream file(loggedInUser + "_logs.txt", ios::app);
    if (file.is_open()) {
        file << entry << "\n";
        file.close();
    }
}

// --- FILE HANDLING: USERS ---

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

// --- FILE HANDLING: INVENTORY (LIBRARY) ---

// RUBRIC: Retrieve inventory records using file handling
void loadLibrary() {
    currentLibrary.clear();
    activityLogs.clear();

    // Load Media
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

    // Load Logs
    ifstream logFile(loggedInUser + "_logs.txt");
    if (logFile.is_open()) {
        string logLine;
        while (getline(logFile, logLine)) {
            activityLogs.push_back(logLine);
        }
        logFile.close();
    }
}

// RUBRIC: Save inventory records using file handling
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
            loadLibrary(); // Load user's personal inventory
            logActivity("User logged in.");
            clearMessage();
            return true;
        }
    }
    authMessage = "Incorrect username or password."; isAuthError = true;
    return false;
}

// --- UI HELPERS ---
void resetForm() {
    inputTitle[0] = '\0';
    snprintf(inputType, sizeof(inputType), "Anime");
    inputCurrent = 0;
    inputTotal = 12;
    inputRating = 0;
    snprintf(inputStatus, sizeof(inputStatus), "Watching");
    editingIndex = -1;
}

// =============================================================================
//  MAIN PROGRAM
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
    style.WindowRounding = 10.0f;
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.15f, 1.0f);

    loadUsers();

    char usernameInput[64] = "";
    char passwordInput[64] = "";
    bool showPassword = false;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame(); ImGui_ImplGlfw_NewFrame(); ImGui::NewFrame();
        ImGuiIO& io = ImGui::GetIO();

        // ---------------------------------------------------------------------
        // AUTHENTICATION SCREENS (LOGIN / SIGNUP)
        // ---------------------------------------------------------------------
        if (currentScreen == LOGIN || currentScreen == SIGNUP) {
            float winW = 600.0f, winH = 550.0f, elemW = 480.0f;
            float offsetX = (winW - elemW) * 0.5f;

            ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(winW, winH));

            ImGui::Begin(currentScreen == LOGIN ? "Login" : "Signup", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
            
            ImGui::Spacing(); ImGui::SetWindowFontScale(3.0f);
            float titleW = ImGui::CalcTextSize("ANILOG").x;
            ImGui::SetCursorPosX((winW - titleW) * 0.5f);
            ImGui::TextColored(ImVec4(0.29f, 0.56f, 1.0f, 1.0f), "ANILOG");
            
            ImGui::SetWindowFontScale(1.4f); 
            ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

            ImGui::SetCursorPosX(offsetX); ImGui::Text(currentScreen == LOGIN ? "Username" : "Choose Username");
            ImGui::SetCursorPosX(offsetX); ImGui::SetNextItemWidth(elemW);
            ImGui::InputText("##user", usernameInput, IM_ARRAYSIZE(usernameInput));
            ImGui::Spacing();

            ImGui::SetCursorPosX(offsetX); ImGui::Text(currentScreen == LOGIN ? "Password" : "Create Password");
            ImGui::SetCursorPosX(offsetX); 
            float toggleBtnW = 80.0f;
            ImGui::SetNextItemWidth(elemW - toggleBtnW - 8.0f);
            ImGuiInputTextFlags passFlags = showPassword ? ImGuiInputTextFlags_None : ImGuiInputTextFlags_Password;
            ImGui::InputText("##pass", passwordInput, IM_ARRAYSIZE(passwordInput), passFlags);
            ImGui::SameLine();
            if (ImGui::Button(showPassword ? "Hide" : "Show", ImVec2(toggleBtnW, 0))) showPassword = !showPassword;

            if (authMessage != "") {
                ImGui::Spacing(); ImGui::SetCursorPosX(offsetX);
                if (isAuthError) ImGui::TextColored(ImVec4(1.0f, 0.38f, 0.38f, 1.0f), "%s", authMessage.c_str());
                else ImGui::TextColored(ImVec4(0.35f, 0.95f, 0.55f, 1.0f), "%s", authMessage.c_str());
            }

            ImGui::Spacing(); ImGui::Spacing();
            ImGui::SetCursorPosX(offsetX);
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
            // --- SIDEBAR (RUBRIC: Menu-driven interface) ---
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(250, io.DisplaySize.y));
            ImGui::Begin("Sidebar", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
            ImGui::SetWindowFontScale(1.4f);
            
            ImGui::TextColored(ImVec4(0.29f, 0.56f, 1.0f, 1.0f), "ANILOG");
            ImGui::Text("User: %s", loggedInUser.c_str());
            ImGui::Separator(); ImGui::Spacing();

            if (ImGui::Button("My Vault", ImVec2(230, 40))) currentTab = LIBRARY;
            if (ImGui::Button("Add Record", ImVec2(230, 40))) { currentTab = ADD_MEDIA; resetForm(); }
            if (ImGui::Button("Activity Log", ImVec2(230, 40))) currentTab = ACTIVITY_LOG;
            
            ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
            if (ImGui::Button("Logout", ImVec2(230, 40))) {
                logActivity("User logged out.");
                currentScreen = LOGIN; loggedInUser = ""; usernameInput[0]='\0'; passwordInput[0]='\0';
            }
            ImGui::End();

            // --- MAIN CONTENT AREA ---
            ImGui::SetNextWindowPos(ImVec2(250, 0));
            ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x - 250, io.DisplaySize.y));
            ImGui::Begin("Content", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
            ImGui::SetWindowFontScale(1.3f);

            // ==========================================
            // TAB 1: LIBRARY (RUBRIC: Display all products)
            // ==========================================
            if (currentTab == LIBRARY) {
                ImGui::Text("My Media Vault");
                
                // RUBRIC: Search for a specific product
                ImGui::InputText("Search Title", searchBuffer, IM_ARRAYSIZE(searchBuffer));
                string searchStr = searchBuffer;
                
                ImGui::Spacing();
                
                if (ImGui::BeginTable("LibraryTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                    ImGui::TableSetupColumn("Title");
                    ImGui::TableSetupColumn("Type");
                    ImGui::TableSetupColumn("Stock/Progress");
                    ImGui::TableSetupColumn("Status");
                    ImGui::TableSetupColumn("Rating");
                    ImGui::TableSetupColumn("Actions");
                    ImGui::TableHeadersRow();

                    for (int i = 0; i < currentLibrary.size(); i++) {
                        // Filter search
                        if (searchStr != "" && currentLibrary[i].title.find(searchStr) == string::npos) continue;

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0); ImGui::Text("%s", currentLibrary[i].title.c_str());
                        ImGui::TableSetColumnIndex(1); ImGui::Text("%s", currentLibrary[i].type.c_str());
                        
                        // RUBRIC: Monitor stock quantities
                        ImGui::TableSetColumnIndex(2); 
                        ImGui::Text("%d / %d", currentLibrary[i].currentProgress, currentLibrary[i].totalProgress);
                        
                        ImGui::TableSetColumnIndex(3); ImGui::Text("%s", currentLibrary[i].status.c_str());
                        ImGui::TableSetColumnIndex(4); ImGui::Text("%d/5", currentLibrary[i].rating);
                        
                        ImGui::TableSetColumnIndex(5);
                        
                        // RUBRIC: Accept and process customer orders (Validate & Deduct stock)
                        ImGui::PushID(i);
                        if (currentLibrary[i].currentProgress < currentLibrary[i].totalProgress) {
                            if (ImGui::Button("Process/Watch", ImVec2(140, 0))) {
                                currentLibrary[i].currentProgress++;
                                logActivity("Order Processed: Watched 1 Ep of " + currentLibrary[i].title);
                                
                                // Auto-complete logic
                                if (currentLibrary[i].currentProgress == currentLibrary[i].totalProgress) {
                                    currentLibrary[i].status = "Completed";
                                    currentLibrary[i].dateFinished = getCurrentDate();
                                    logActivity("Inventory Emptied: Completed " + currentLibrary[i].title);
                                }
                                saveLibrary();
                            }
                        } else {
                            // If completed, allow "Reread/Rewatch"
                            if (ImGui::Button("Rewatch System", ImVec2(140, 0))) {
                                currentLibrary[i].rereadCount++;
                                currentLibrary[i].currentProgress = 0; // Reset progress
                                currentLibrary[i].status = "Watching";
                                logActivity("Restocked/Rewatching: " + currentLibrary[i].title);
                                saveLibrary();
                            }
                        }

                        ImGui::SameLine();
                        // RUBRIC: Update existing product information
                        if (ImGui::Button("Edit")) {
                            editingIndex = i;
                            snprintf(inputTitle, sizeof(inputTitle), "%s", currentLibrary[i].title.c_str());
                            snprintf(inputType, sizeof(inputType), "%s", currentLibrary[i].type.c_str());
                            inputCurrent = currentLibrary[i].currentProgress;
                            inputTotal = currentLibrary[i].totalProgress;
                            inputRating = currentLibrary[i].rating;
                            snprintf(inputStatus, sizeof(inputStatus), "%s", currentLibrary[i].status.c_str());
                            currentTab = EDIT_MEDIA;
                        }

                        ImGui::SameLine();
                        // RUBRIC: Delete product records
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
                        if (ImGui::Button("Delete")) {
                            logActivity("Deleted Record: " + currentLibrary[i].title);
                            currentLibrary.erase(currentLibrary.begin() + i);
                            saveLibrary();
                        }
                        ImGui::PopStyleColor();
                        ImGui::PopID();
                    }
                    ImGui::EndTable();
                }
            }
            
            // ==========================================
            // TAB 2: ADD / EDIT MEDIA (RUBRIC: Add new products)
            // ==========================================
            else if (currentTab == ADD_MEDIA || currentTab == EDIT_MEDIA) {
                ImGui::Text(currentTab == ADD_MEDIA ? "Add New Inventory Record" : "Edit Inventory Record");
                ImGui::Separator(); ImGui::Spacing();

                ImGui::InputText("Title", inputTitle, IM_ARRAYSIZE(inputTitle));
                
                // Simplified Type & Status using InputText for basic C++ (No complex ComboBox arrays)
                ImGui::InputText("Type (Anime/Manga)", inputType, IM_ARRAYSIZE(inputType));
                ImGui::InputInt("Current Progress", &inputCurrent);
                ImGui::InputInt("Total Episodes/Chapters", &inputTotal);
                ImGui::InputInt("Rating (1-5)", &inputRating);
                ImGui::InputText("Status", inputStatus, IM_ARRAYSIZE(inputStatus));

                ImGui::Spacing();
                
                if (ImGui::Button("Save Record", ImVec2(200, 50))) {
                    if (string(inputTitle) != "") {
                        if (currentTab == ADD_MEDIA) {
                            MediaRecord newMedia;
                            newMedia.title = inputTitle;
                            newMedia.type = inputType;
                            newMedia.currentProgress = inputCurrent;
                            newMedia.totalProgress = inputTotal;
                            newMedia.rating = inputRating;
                            newMedia.status = inputStatus;
                            newMedia.dateStarted = getCurrentDate();
                            newMedia.dateFinished = "";
                            newMedia.rereadCount = 0;
                            
                            currentLibrary.push_back(newMedia);
                            logActivity("Added New Product: " + newMedia.title);
                        } else {
                            currentLibrary[editingIndex].title = inputTitle;
                            currentLibrary[editingIndex].type = inputType;
                            currentLibrary[editingIndex].currentProgress = inputCurrent;
                            currentLibrary[editingIndex].totalProgress = inputTotal;
                            currentLibrary[editingIndex].rating = inputRating;
                            currentLibrary[editingIndex].status = inputStatus;
                            logActivity("Updated Product Info: " + string(inputTitle));
                        }
                        saveLibrary();
                        currentTab = LIBRARY;
                        resetForm();
                    }
                }
            }

            // ==========================================
            // TAB 3: ACTIVITY LOG (RUBRIC: Generate Receipts)
            // ==========================================
            else if (currentTab == ACTIVITY_LOG) {
                ImGui::Text("Transaction Summary & Order Receipts");
                ImGui::Separator(); ImGui::Spacing();

                // Scrollable child area
                ImGui::BeginChild("LogScroll", ImVec2(0, 0), true);
                for (int i = activityLogs.size() - 1; i >= 0; i--) {
                    ImGui::Text("%s", activityLogs[i].c_str());
                }
                ImGui::EndChild();
            }

            ImGui::End();
        }

        ImGui::Render();
        glClearColor(0.07f, 0.07f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}