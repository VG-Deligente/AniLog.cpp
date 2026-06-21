// =============================================================================
//  anilog_activity_log.cpp
//  TAB 4: Activity Log - newest-first entries grouped by date.
// =============================================================================

#include "anilog_globals.h"

void RenderActivityLogTab() {
    // Heading and empty state. Activity entries are loaded during login and
    // appended by logActivity() whenever important user actions happen.
    ImGui::SetWindowFontScale(FONT_SCALE_HEADER);
    ImGui::TextColored(COLOR_ACCENT_BLUE, "Activity Log");
    ImGui::Separator(); ImGui::Spacing();

    ImGui::SetWindowFontScale(FONT_SCALE_BODY);
    if (activityLogs.empty()) {
        ImGui::TextColored(COLOR_MUTED, "No activity recorded yet.");
        return;
    }

    // Scrollable region lets the log grow indefinitely without pushing the
    // dashboard layout out of place.
    ImGui::BeginChild("LogScroll", ImVec2(0, 0), true);
    ImGui::SetWindowFontScale(FONT_SCALE_BODY);

    string lastDate = "";

    // Render newest first. Dates are parsed from "[YYYY-MM-DD]" prefixes so
    // entries from the same day can be grouped under one divider.
    for (int i = (int)activityLogs.size() - 1; i >= 0; i--) {
        const string& log = activityLogs[i];

        string date = "";
        if (log.size() > 12 && log[0] == '[')
            date = log.substr(1, 10);

        if (date != lastDate) {
            lastDate = date;
            if (i < (int)activityLogs.size() - 1)
                ImGui::Spacing();
            string divider = "----- " + (date.empty() ? "Unknown Date" : date) + " -----";
            ImGui::TextColored(ImVec4(0.5f, 0.75f, 1.0f, 0.9f), "%s", divider.c_str());
            ImGui::Spacing();
        }

        // Hide the date prefix from each row because the date divider already
        // provides that context.
        string displayLog = log;
        if (log.size() > 13 && log[0] == '[')
            displayLog = log.substr(13);

        ImGui::TextWrapped("%s", displayLog.c_str());
        ImGui::Separator();
    }

    ImGui::EndChild();
}
