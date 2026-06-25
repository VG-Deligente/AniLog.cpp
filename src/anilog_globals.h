#pragma once
// =============================================================================
//  anilog_globals.h
// -----------------------------------------------------------------------------
//  WHAT THIS FILE IS
//    This is the single header every other source file includes. It is the
//    "shared vocabulary" of the program: the type scale, the accent colors,
//    the data structures (a user, a media record), the list of screens/tabs,
//    and the declarations of every global variable and function that more than
//    one file needs to touch. Nothing here does work on its own - it simply
//    tells every .cpp file what exists and what its name and shape are.
//
//  WHY IT MATTERS
//    Because every tab includes this header, anything defined here is applied
//    consistently across the entire interface. That makes this the correct,
//    one-stop place to change look-and-feel decisions that should stay uniform
//    (text sizes and accent colors), instead of hunting through each tab.
//
//  HOW TO NAVIGATE / WHAT YOU CAN SAFELY CHANGE
//    - Text sizes for the whole app ....... edit the FONT_SCALE_* values below.
//    - Shared accent colors ............... edit the COLOR_* values below.
//    - What a stored record looks like .... edit the MediaRecord struct.
//    - The set of screens and tabs ........ edit the Screen / DashboardTab enums.
//    Declarations marked "extern" are only *announced* here; their actual
//    storage lives in anilog_utils.cpp / main.cpp. Changing a name here means
//    changing it there too.
// =============================================================================

#include "imgui.h"
#include <string>
#include <vector>
using namespace std;

// =============================================================================
//  UI TEXT-SIZE HIERARCHY  -  THE ONE PLACE THAT CONTROLS ALL TEXT SIZES
// -----------------------------------------------------------------------------
//  WHAT THESE DO
//    Every tab calls ImGui::SetWindowFontScale(FONT_SCALE_*) instead of typing
//    raw numbers, so all five tabs share the exact same type scale. The numbers
//    are *multipliers* on the base font: 1.0f is the font's natural size, 2.0f
//    is double. Four named levels keep the hierarchy consistent everywhere:
//      HEADER  -> the single big title at the very top of each tab.
//      SECTION -> sub-section headings inside a tab (e.g. "Active", "Progress Metrics").
//      BODY    -> the default reading size: table cells, form fields, paragraphs.
//      SMALL   -> tiny inline notes only (e.g. a "3 / 5" rating counter).
//
//  HOW TO RESIZE TEXT ACROSS THE ENTIRE APP
//    Change the four numbers below. Bigger numbers = bigger text everywhere,
//    instantly and uniformly - you never edit the individual tabs. Keep them in
//    descending order (HEADER > SECTION > BODY > SMALL) so the visual hierarchy
//    stays intact. The values below are deliberately larger than the original
//    defaults to improve readability; the gaps between levels are kept modest
//    so the bigger text still feels balanced and does not overflow rows.
//    (Note: the left sidebar and the login screen set their own scale directly
//     in main.cpp, because they are drawn in a different window.)
// =============================================================================
#define FONT_SCALE_HEADER 1.9f // tab titles
#define FONT_SCALE_SECTION 1.6f // section sub-headings
#define FONT_SCALE_BODY 1.45f // default body text
#define FONT_SCALE_SMALL 1.2f // minor inline counters

// =============================================================================
//  SHARED ACCENT COLORS - reused across tabs so the same kind of element
//  (titles, success states, warnings, danger actions) always reads the same.
// =============================================================================
#define COLOR_ACCENT_BLUE ImVec4(0.35f, 0.65f, 1.0f, 1.0f)
#define COLOR_ACCENT_GREEN ImVec4(0.4f, 1.0f, 0.6f, 1.0f)
#define COLOR_ACCENT_ORANGE ImVec4(1.0f, 0.75f, 0.3f, 1.0f)
#define COLOR_ACCENT_RED ImVec4(1.0f, 0.4f, 0.4f, 1.0f)
#define COLOR_ACCENT_PURPLE ImVec4(0.9f, 0.5f, 1.0f, 1.0f)
#define COLOR_MUTED ImVec4(0.55f, 0.55f, 0.55f, 1.0f)
#define COLOR_LABEL ImVec4(0.7f, 0.7f, 0.7f, 1.0f)

// =============================================================================
//  ENUMS
// =============================================================================
enum Screen { LOGIN, SIGNUP, DASHBOARD };
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
    int currentProgress;
    int totalProgress;
    int rating;
    string status;
    string dateStarted;
    string dateFinished;
    int rereadCount;
};

// =============================================================================
//  EXTERN GLOBALS - defined in main.cpp, accessible everywhere via this header
// =============================================================================

// Data collections
extern vector<UserRecord> userDatabase; // list of user accounts
extern vector<MediaRecord> currentLibrary; // list of all media for the logged-in user
extern vector<string> activityLogs; //activity log entries for the logged-in user

// Session state
extern string loggedInUser; // If the user is logged in
extern string authMessage; // If the user is not logged in, this contains the error message to display on the login screen.
extern bool isAuthError; // it is used to color message red for error and green for success

// Screen / tab state
extern Screen currentScreen; // Tells if the screen is LOGIN, SIGNUP, OR DASHBOARD.
extern DashboardTab currentTab; // Tells which tab is currently active in the dashboard.

// Add / Edit form buffers (temporary storage)
extern char inputTitle[128];
extern int inputTypeIndex; // anime or manga
extern int inputCurrent; // current episode or chapter
extern int inputTotal; // total episode or chapter
extern int inputRating; // 0-5 stars
extern int inputStatusIndex; // watching, completed, on-hold, dropped, plan to watch
extern int editingIndex; // tells which entry in library you're editing

// Library filter / search
extern char searchBuffer[128]; // what you type in search box
extern int currentFilterIndex; // index of the currently selected filter

// UI state flags (It help shows confirmation popups)
extern bool pendingNavAway; // for discard confirmation when switching tabs with unsaved changes
extern DashboardTab pendingTab; // the tab you're navigating to
extern bool showRewatchConfirm; // whether to show the rewatch confirmation dialog
extern int rewatchTargetIndex; // the index of the record being rewatched

// String lookup tables
extern const char* typeOptions[]; // "Anime" or "Manga"
extern const char* animeStatusOptions[]; // "Watching", "Completed", "On-Hold", "Dropped", "Plan to Watch"
extern const char* mangaStatusOptions[]; // "Reading", "Completed", "On-Hold", "Dropped", "Plan to Read"
extern const char* filterOptions[3]; // "All", "Watching/Reading", "Completed"

// =============================================================================
//  UTILITY FUNCTION DECLARATIONS - defined in anilog_utils.cpp
// =============================================================================
string getCurrentDate(); // Returns the current date in YYYY-MM-DD format.
string toLowerStr(const string& s); // Converts a string to lowercase (used for search so "A" matches "a")
string trimStr(const string& s); // Removes leading and trailing whitespace from a string
void logActivity(const string& action); // Adds a new entry to the activity log with the current date and time.
string buildChangeSummary(const MediaRecord& before, const MediaRecord& after); // Compares two MediaRecord objects and returns a string summarizing the changes made.
void saveLibrary(); // Saves the current state of the library to disk.
void resetForm(); // Clears all fields in the add/edit form.
bool titleExists(const string& title, int excludeIndex = -1); // Checks if a media item with the given title already exists in the library.

// =============================================================================
//  SORTING COMPARATORS - defined in anilog_utils.cpp
// =============================================================================
bool sortTitleAsc(const MediaRecord& a, const MediaRecord& b);
bool sortTitleDesc(const MediaRecord& a, const MediaRecord& b);
bool sortRatingDesc(const MediaRecord& a, const MediaRecord& b);
bool sortProgressDesc(const MediaRecord& a, const MediaRecord& b);

// =============================================================================
//  TAB RENDER FUNCTION DECLARATIONS - defined in their respective .cpp files
// =============================================================================
void RenderLibraryTab(ImVec2 center);
void RenderAddEditTab(ImVec2 center);
void RenderStatisticsTab();
void RenderActivityLogTab();
void RenderHelpTab();
