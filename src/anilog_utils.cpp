// =============================================================================
//  anilog_utils.cpp
//  Utility functions, file I/O, authentication, sorting comparators.
//  All globals are DEFINED here (not just declared).
// =============================================================================

#include "anilog_globals.h"
#include <fstream>
#include <sstream>
#include <ctime>
#include <algorithm>
#include <cctype>

// =============================================================================
//  GLOBAL DEFINITIONS - storage lives here, extern in anilog_globals.h
// =============================================================================
vector<UserRecord>  userDatabase;
vector<MediaRecord> currentLibrary;
vector<string>      activityLogs;

string loggedInUser = "";
string authMessage  = "";
bool   isAuthError  = false;

Screen       currentScreen = LOGIN;
DashboardTab currentTab    = LIBRARY;

char inputTitle[128] = "";
int  inputTypeIndex  = 0;
int  inputCurrent    = 0;
int  inputTotal      = 12;
int  inputRating     = 3;
int  inputStatusIndex= 0;
int  editingIndex    = -1;

char searchBuffer[128]  = "";
int  currentFilterIndex = 0;

bool         pendingNavAway = false;
DashboardTab pendingTab     = LIBRARY;
bool         showRewatchConfirm  = false;
int          rewatchTargetIndex  = -1;

const char* typeOptions[]        = { "Anime", "Manga" };
const char* animeStatusOptions[] = { "Watching", "Completed", "Dropped" };
const char* mangaStatusOptions[] = { "Reading",  "Completed", "Dropped" };
const char* filterOptions[]      = { "All Media", "Anime Only", "Manga Only" };

// =============================================================================
//  UTILITY FUNCTIONS
// =============================================================================
string getCurrentDate() {
    time_t now = time(0);
    tm* ltm = localtime(&now);
    char dateStr[11];
    snprintf(dateStr, sizeof(dateStr), "%04d-%02d-%02d",
             1900 + ltm->tm_year, 1 + ltm->tm_mon, ltm->tm_mday);
    return string(dateStr);
}

string toLowerStr(const string& s) {
    string result = s;
    for (int i = 0; i < (int)result.size(); i++)
        result[i] = (char)tolower((unsigned char)result[i]);
    return result;
}

string trimStr(const string& s) {
    int start = 0, end = (int)s.size() - 1;
    while (start <= end && s[start] == ' ') start++;
    while (end >= start && s[end]   == ' ') end--;
    return s.substr(start, end - start + 1);
}

void logActivity(const string& action) {
    string entry = "[" + getCurrentDate() + "] " + action;
    activityLogs.push_back(entry);
    ofstream file(loggedInUser + "_logs.txt", ios::app);
    if (file.is_open()) { file << entry << "\n"; file.close(); }
}

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
    if (changes.size() >= 3 && changes.substr(changes.size() - 3) == " | ")
        changes = changes.substr(0, changes.size() - 3);
    return changes.empty() ? "No changes detected." : changes;
}

// =============================================================================
//  SORTING COMPARATORS
// =============================================================================
bool sortTitleAsc  (const MediaRecord& a, const MediaRecord& b) { return a.title < b.title; }
bool sortTitleDesc (const MediaRecord& a, const MediaRecord& b) { return a.title > b.title; }
bool sortRatingDesc(const MediaRecord& a, const MediaRecord& b) { return a.rating > b.rating; }
bool sortProgressDesc(const MediaRecord& a, const MediaRecord& b) {
    float pctA = (a.totalProgress > 0) ? (float)a.currentProgress / a.totalProgress : 0;
    float pctB = (b.totalProgress > 0) ? (float)b.currentProgress / b.totalProgress : 0;
    return pctA > pctB;
}

// =============================================================================
//  FILE HANDLING
// =============================================================================
void loadUsers() {
    ifstream file("users.txt");
    if (!file.is_open()) return;
    UserRecord temp;
    while (getline(file, temp.username, ',') && getline(file, temp.password))
        userDatabase.push_back(temp);
    file.close();
}

void saveUser(UserRecord u) {
    ofstream file("users.txt", ios::app);
    if (file.is_open()) { file << u.username << "," << u.password << "\n"; file.close(); }
}

void loadLibrary() {
    currentLibrary.clear();
    activityLogs.clear();

    ifstream file(loggedInUser + "_library.txt");
    if (file.is_open()) {
        string line;
        while (getline(file, line)) {
            stringstream ss(line);
            MediaRecord m;
            string val;
            try {
                getline(ss, m.title,        '|');
                getline(ss, m.type,         '|');
                getline(ss, val,            '|'); m.currentProgress = stoi(val);
                getline(ss, val,            '|'); m.totalProgress   = stoi(val);
                getline(ss, val,            '|'); m.rating          = stoi(val);
                getline(ss, m.status,       '|');
                getline(ss, m.dateStarted,  '|');
                getline(ss, m.dateFinished, '|');
                getline(ss, val);                 m.rereadCount     = stoi(val);

                if (m.totalProgress   < 1) m.totalProgress   = 1;
                if (m.currentProgress < 0) m.currentProgress = 0;
                if (m.currentProgress > m.totalProgress) m.currentProgress = m.totalProgress;
                if (m.rating < 1) m.rating = 1;
                if (m.rating > 5) m.rating = 5;
                if (m.rereadCount < 0) m.rereadCount = 0;
                currentLibrary.push_back(m);
            } catch (...) {}
        }
        file.close();
    }

    ifstream logFile(loggedInUser + "_logs.txt");
    if (logFile.is_open()) {
        string logLine;
        while (getline(logFile, logLine))
            activityLogs.push_back(logLine);
        logFile.close();
    }
}

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
//  AUTHENTICATION
// =============================================================================
void clearMessage() { authMessage = ""; isAuthError = false; }

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

void resetForm() {
    inputTitle[0]    = '\0';
    inputTypeIndex   = 0;
    inputCurrent     = 0;
    inputTotal       = 12;
    inputRating      = 3;
    inputStatusIndex = 0;
    editingIndex     = -1;
}

bool titleExists(const string& title, int excludeIndex) {
    string lowerNew = toLowerStr(trimStr(title));
    for (int i = 0; i < (int)currentLibrary.size(); i++) {
        if (i == excludeIndex) continue;
        if (toLowerStr(currentLibrary[i].title) == lowerNew) return true;
    }
    return false;
}