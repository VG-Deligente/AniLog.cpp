#pragma once
// =============================================================================
//  anilog_globals.h
//  Shared enums, structs, and extern declarations for all translation units.
//  Include this in every .cpp file that needs access to global state.
// =============================================================================

#include "imgui.h"
#include <string>
#include <vector>
using namespace std;

// =============================================================================
//  ENUMS
// =============================================================================
enum Screen       { LOGIN, SIGNUP, DASHBOARD };
enum DashboardTab { LIBRARY, ADD_MEDIA, EDIT_MEDIA, ACTIVITY_LOG, STATISTICS, HELP };

// =============================================================================
//  DATA STRUCTURES
// =============================================================================
struct UserRecord {
    string username;
    string password;
};

struct MediaRecord {
    string title;
    string type;
    int    currentProgress;
    int    totalProgress;
    int    rating;
    string status;
    string dateStarted;
    string dateFinished;
    int    rereadCount;
};

// =============================================================================
//  EXTERN GLOBALS — defined in main.cpp, accessible everywhere via this header
// =============================================================================

// Data collections
extern vector<UserRecord>  userDatabase;
extern vector<MediaRecord> currentLibrary;
extern vector<string>      activityLogs;

// Session state
extern string loggedInUser;
extern string authMessage;
extern bool   isAuthError;

// Screen / tab state
extern Screen       currentScreen;
extern DashboardTab currentTab;

// Add / Edit form buffers
extern char inputTitle[128];
extern int  inputTypeIndex;
extern int  inputCurrent;
extern int  inputTotal;
extern int  inputRating;
extern int  inputStatusIndex;
extern int  editingIndex;

// Library filter / search
extern char searchBuffer[128];
extern int  currentFilterIndex;

// UI state flags
extern bool         pendingNavAway;
extern DashboardTab pendingTab;
extern bool         showRewatchConfirm;
extern int          rewatchTargetIndex;

// String lookup tables
extern const char* typeOptions[];
extern const char* animeStatusOptions[];
extern const char* mangaStatusOptions[];
extern const char* filterOptions[3];

// =============================================================================
//  UTILITY FUNCTION DECLARATIONS — defined in anilog_utils.cpp
// =============================================================================
string getCurrentDate();
string toLowerStr(const string& s);
string trimStr(const string& s);
void   logActivity(const string& action);
string buildChangeSummary(const MediaRecord& before, const MediaRecord& after);
void   saveLibrary();
void   resetForm();
bool   titleExists(const string& title, int excludeIndex = -1);

// =============================================================================
//  SORTING COMPARATORS — defined in anilog_utils.cpp
// =============================================================================
bool sortTitleAsc    (const MediaRecord& a, const MediaRecord& b);
bool sortTitleDesc   (const MediaRecord& a, const MediaRecord& b);
bool sortRatingDesc  (const MediaRecord& a, const MediaRecord& b);
bool sortProgressDesc(const MediaRecord& a, const MediaRecord& b);

// =============================================================================
//  TAB RENDER FUNCTION DECLARATIONS — defined in their respective .cpp files
// =============================================================================
void RenderLibraryTab    (ImVec2 center);
void RenderAddEditTab    (ImVec2 center);
void RenderStatisticsTab ();
void RenderActivityLogTab();
void RenderHelpTab       ();
