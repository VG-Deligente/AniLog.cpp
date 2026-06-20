// =============================================================================
//  anilog_activity_log.cpp
//  TAB 4: Activity Log — newest-first entries grouped by date.
// =============================================================================

#include "anilog_globals.h"

void RenderActivityLogTab() {
    ImGui::SetWindowFontScale(1.5f);
    ImGui::Text("Activity Log");
    ImGui::Separator(); ImGui::Spacing();

    ImGui::SetWindowFontScale(1.3f);
    if (activityLogs.empty()) {
        ImGui::TextColored(ImVec4(0.55f, 0.55f, 0.55f, 1.0f), "No activity recorded yet.");
        return;
    }

    ImGui::BeginChild("LogScroll", ImVec2(0, 0), true);
    ImGui::SetWindowFontScale(1.3f);

    string lastDate = "";

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

        string displayLog = log;
        if (log.size() > 13 && log[0] == '[')
            displayLog = log.substr(13);

        ImGui::TextWrapped("%s", displayLog.c_str());
        ImGui::Separator();
    }

    ImGui::EndChild();
}
