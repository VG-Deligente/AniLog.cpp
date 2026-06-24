// =============================================================================
//  anilog_library.cpp
//  TAB 1: AniDex - Library view with Active, Completed, and Dropped tables.
// =============================================================================

#include "anilog_globals.h"
#include <algorithm>

void RenderLibraryTab(ImVec2 center) {
    // Tab heading and common table styling. All three sections below reuse
    // tableFlags so borders, row striping, and sizing stay visually consistent.
    ImGui::SetWindowFontScale(FONT_SCALE_HEADER);
    ImGui::TextColored(COLOR_ACCENT_BLUE, "AniDex - My Media Vault");
    ImGui::SetWindowFontScale(FONT_SCALE_BODY);
    ImGui::Separator(); ImGui::Spacing();

    // -- Search / filter / sort toolbar --
    // Search and type filters affect only what rows are shown. Sort buttons
    // reorder currentLibrary itself, so the new order remains while switching tabs.
    ImGui::SetNextItemWidth(300);
    ImGui::InputTextWithHint("##search", "Search title...", searchBuffer, IM_ARRAYSIZE(searchBuffer));
    string searchStr = toLowerStr(searchBuffer);

    ImGui::SameLine();
    ImGui::SetNextItemWidth(160);
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

    ImGuiTableFlags tableFlags = ImGuiTableFlags_Borders
        | ImGuiTableFlags_RowBg
        | ImGuiTableFlags_SizingStretchProp;

    // Shared row predicate for all three tables. A blank status means "match
    // any non-final active status"; explicit statuses are used by Completed
    // and Dropped sections.
    auto passesFilter = [&](int i, const string& status) -> bool {
        if (!status.empty() && currentLibrary[i].status != status) return false;
        if (currentFilterIndex == 1 && currentLibrary[i].type != "Anime") return false;
        if (currentFilterIndex == 2 && currentLibrary[i].type != "Manga") return false;
        if (searchStr != "" && toLowerStr(currentLibrary[i].title).find(searchStr) == string::npos) return false;
        return true;
    };

    // -- ACTIVE TABLE --
    // Active records are everything not Completed and not Dropped. This table
    // has the quick +1 progress action because these are the titles currently
    // being watched/read.
    ImGui::SetWindowFontScale(FONT_SCALE_SECTION);
    ImGui::TextColored(COLOR_ACCENT_BLUE, "Active");
    ImGui::SetWindowFontScale(FONT_SCALE_BODY);
    ImGui::Spacing();

    int activeCount = 0;
    for (int i = 0; i < (int)currentLibrary.size(); i++) {
        if (passesFilter(i, "") && currentLibrary[i].status != "Completed" && currentLibrary[i].status != "Dropped") 
            activeCount++;
    }

    if (activeCount == 0) {
        ImGui::TextColored(ImVec4(0.55f, 0.55f, 0.55f, 1.0f), "No active records. Add one with 'Add Record'!");
        ImGui::Spacing();
    } else if (ImGui::BeginTable("LibraryTable", 6, tableFlags)) {
        ImGui::TableSetupColumn("Title", ImGuiTableColumnFlags_WidthStretch, 2.5f);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableSetupColumn("Progress", ImGuiTableColumnFlags_WidthStretch, 1.2f);
        ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthStretch, 1.5f);
        ImGui::TableSetupColumn("Rating", ImGuiTableColumnFlags_WidthStretch, 0.8f);
        ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthStretch, 2.5f);
        ImGui::TableHeadersRow();

        // Deletion is deferred until after the table loop. Erasing from
        // currentLibrary while iterating would invalidate indices still in use.
        int deleteTarget = -1;

        for (int i = 0; i < (int)currentLibrary.size(); i++) {
            if (!passesFilter(i, "") || currentLibrary[i].status == "Completed" || currentLibrary[i].status == "Dropped") continue;

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::TextWrapped("%s", currentLibrary[i].title.c_str());
            ImGui::TableSetColumnIndex(1); ImGui::Text("%s", currentLibrary[i].type.c_str());
            ImGui::TableSetColumnIndex(2); ImGui::Text("%d / %d", currentLibrary[i].currentProgress, currentLibrary[i].totalProgress);
            ImGui::TableSetColumnIndex(3); ImGui::Text("%s", currentLibrary[i].status.c_str());
            ImGui::TableSetColumnIndex(4); ImGui::Text("%d/5", currentLibrary[i].rating);
            ImGui::TableSetColumnIndex(5);

            ImGui::PushID(i);

            // Quick progress update. When progress reaches the total, the record
            // automatically moves to Completed and records the finish date.
            if (currentLibrary[i].currentProgress < currentLibrary[i].totalProgress) {
                string btnLabel = (currentLibrary[i].type == "Anime") ? "+1 Episode" : "+1 Chapter";
                if (ImGui::Button(btnLabel.c_str(), ImVec2(130, 0))) {
                    currentLibrary[i].currentProgress++;
                    string unit = (currentLibrary[i].type == "Anime") ? "episode" : "chapter";
                    logActivity("Progress Updated: [" + currentLibrary[i].title
                              + "] advanced to " + unit + " "
                              + to_string(currentLibrary[i].currentProgress)
                              + " of " + to_string(currentLibrary[i].totalProgress));
                    if (currentLibrary[i].currentProgress == currentLibrary[i].totalProgress) {
                        currentLibrary[i].status = "Completed";
                        currentLibrary[i].dateFinished = getCurrentDate();
                        logActivity("Status Changed: [" + currentLibrary[i].title + "] marked as Completed.");
                    }
                    saveLibrary();
                }
                ImGui::SameLine();
            }

            // Copy the selected row into the shared Add/Edit buffers, then let
            // RenderAddEditTab handle the actual editing screen.
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
            ImGui::SameLine();

            // Destructive actions use a per-row popup id so multiple rows can
            // safely render their own confirmation dialog in the same table.
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
            string delPopupId = "DelConf_Active_" + to_string(i);
            if (ImGui::Button("Delete"))
                ImGui::OpenPopup(delPopupId.c_str());
            ImGui::PopStyleColor(2);

            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            if (ImGui::BeginPopupModal(delPopupId.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("Delete \"%s\"?", currentLibrary[i].title.c_str());
                ImGui::Text("This action cannot be undone.");
                ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
                if (ImGui::Button("Yes, Delete", ImVec2(120, 35))) { deleteTarget = i; ImGui::CloseCurrentPopup(); }
                ImGui::PopStyleColor(2);
                ImGui::SameLine(); ImGui::Spacing(); ImGui::SameLine();
                if (ImGui::Button("Cancel", ImVec2(120, 35))) ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
            }

            ImGui::PopID();
        }
        ImGui::EndTable();

        if (deleteTarget >= 0) {
            logActivity("Record Deleted: [" + currentLibrary[deleteTarget].title + "] removed from the library.");
            currentLibrary.erase(currentLibrary.begin() + deleteTarget);
            saveLibrary();
        }
    }

    ImGui::Spacing(); ImGui::Spacing();

    // -- COMPLETED TABLE --
    // Completed records show date finished and rewatch/reread count. Restarting
    // a title moves it back to Active while preserving history through rereadCount.
    ImGui::SetWindowFontScale(FONT_SCALE_SECTION);
    ImGui::TextColored(COLOR_ACCENT_GREEN, "Completed");
    ImGui::SetWindowFontScale(FONT_SCALE_BODY);
    ImGui::Spacing();

    int completedCount = 0;
    for (int i = 0; i < (int)currentLibrary.size(); i++) {
        if (passesFilter(i, "Completed")) completedCount++;
    }

    if (completedCount == 0) {
        ImGui::TextColored(ImVec4(0.55f, 0.55f, 0.55f, 1.0f), "No completed titles yet. Keep watching!");
        ImGui::Spacing();
    } else if (ImGui::BeginTable("CompletedTable", 6, tableFlags)) {
        ImGui::TableSetupColumn("Title", ImGuiTableColumnFlags_WidthStretch, 2.5f);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableSetupColumn("Date Finished", ImGuiTableColumnFlags_WidthStretch, 1.5f);
        ImGui::TableSetupColumn("Rating", ImGuiTableColumnFlags_WidthStretch, 0.8f);
        ImGui::TableSetupColumn("Rewatched/Reread", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthStretch, 2.5f);
        ImGui::TableHeadersRow();

        // Same deferred-delete pattern as the Active table.
        int completedDelete_target = -1;

        for (int i = 0; i < (int)currentLibrary.size(); i++) {
            if (!passesFilter(i, "Completed")) continue;

            ImGui::PushID(200 + i);
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::TextWrapped("%s", currentLibrary[i].title.c_str());
            ImGui::TableSetColumnIndex(1); ImGui::Text("%s", currentLibrary[i].type.c_str());
            ImGui::TableSetColumnIndex(2); ImGui::Text("%s", currentLibrary[i].dateFinished.empty() ? "-" : currentLibrary[i].dateFinished.c_str());
            ImGui::TableSetColumnIndex(3); ImGui::Text("%d/5", currentLibrary[i].rating);
            ImGui::TableSetColumnIndex(4); ImGui::Text("%d", currentLibrary[i].rereadCount);
            ImGui::TableSetColumnIndex(5);

            // Restart keeps the record but resets progress, clears the finish
            // date, and increments the lifetime rewatch/reread counter.
            string rBtnLabel = (currentLibrary[i].type == "Anime") ? "Rewatch" : "Reread";
            string rPopupId = "RewatchConf_" + to_string(i);
            if (ImGui::Button(rBtnLabel.c_str(), ImVec2(110, 0)))
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
                if (ImGui::Button("Cancel", ImVec2(100, 35))) ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
            }

            ImGui::SameLine();
            // Completed rows always preload the status picker to Completed.
            if (ImGui::Button("Edit")) {
                editingIndex = i;
                snprintf(inputTitle, sizeof(inputTitle), "%s", currentLibrary[i].title.c_str());
                inputTypeIndex = (currentLibrary[i].type == "Anime") ? 0 : 1;
                inputCurrent = currentLibrary[i].currentProgress;
                inputTotal = currentLibrary[i].totalProgress;
                inputRating = currentLibrary[i].rating;
                inputStatusIndex = 1;
                currentTab = EDIT_MEDIA;
            }

            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
            string cDelPopupId = "DelConf_Completed_" + to_string(i);
            if (ImGui::Button("Delete")) ImGui::OpenPopup(cDelPopupId.c_str());
            ImGui::PopStyleColor(2);

            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            if (ImGui::BeginPopupModal(cDelPopupId.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("Delete \"%s\"?", currentLibrary[i].title.c_str());
                ImGui::Text("This action cannot be undone.");
                ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
                if (ImGui::Button("Yes, Delete", ImVec2(120, 35))) { completedDelete_target = i; ImGui::CloseCurrentPopup(); }
                ImGui::PopStyleColor(2);
                ImGui::SameLine(); ImGui::Spacing(); ImGui::SameLine();
                if (ImGui::Button("Cancel", ImVec2(120, 35))) ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
            }

            ImGui::PopID();
        }
        ImGui::EndTable();

        if (completedDelete_target >= 0) {
            logActivity("Record Deleted: [" + currentLibrary[completedDelete_target].title + "] removed from the library.");
            currentLibrary.erase(currentLibrary.begin() + completedDelete_target);
            saveLibrary();
        }
    }

    ImGui::Spacing(); ImGui::Spacing();

    // -- DROPPED TABLE --
    // Dropped records are separated so they do not clutter Active, but Resume
    // can move them back without opening the edit form.
    ImGui::SetWindowFontScale(FONT_SCALE_SECTION);
    ImGui::TextColored(COLOR_ACCENT_ORANGE, "Dropped");
    ImGui::SetWindowFontScale(FONT_SCALE_BODY);
    ImGui::Spacing();

    int droppedCount = 0;
    for (int i = 0; i < (int)currentLibrary.size(); i++) {
        if (passesFilter(i, "Dropped")) droppedCount++;
    }

    if (droppedCount == 0) {
        ImGui::TextColored(ImVec4(0.55f, 0.55f, 0.55f, 1.0f), "Nothing dropped. Good discipline!");
        ImGui::Spacing();
    } else if (ImGui::BeginTable("DroppedTable", 6, tableFlags)) {
        ImGui::TableSetupColumn("Title", ImGuiTableColumnFlags_WidthStretch, 2.5f);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableSetupColumn("Progress", ImGuiTableColumnFlags_WidthStretch, 1.2f);
        ImGui::TableSetupColumn("Rating", ImGuiTableColumnFlags_WidthStretch, 0.8f);
        ImGui::TableSetupColumn("Started", ImGuiTableColumnFlags_WidthStretch, 1.5f);
        ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthStretch, 2.5f);
        ImGui::TableHeadersRow();

        // Same deferred-delete pattern as the other tables.
        int droppedDelete_target = -1;

        for (int i = 0; i < (int)currentLibrary.size(); i++) {
            if (!passesFilter(i, "Dropped")) continue;

            ImGui::PushID(400 + i);
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::TextWrapped("%s", currentLibrary[i].title.c_str());
            ImGui::TableSetColumnIndex(1); ImGui::Text("%s", currentLibrary[i].type.c_str());
            ImGui::TableSetColumnIndex(2); ImGui::Text("%d / %d", currentLibrary[i].currentProgress, currentLibrary[i].totalProgress);
            ImGui::TableSetColumnIndex(3); ImGui::Text("%d/5", currentLibrary[i].rating);
            ImGui::TableSetColumnIndex(4); ImGui::Text("%s", currentLibrary[i].dateStarted.empty() ? "-" : currentLibrary[i].dateStarted.c_str());
            ImGui::TableSetColumnIndex(5);

            // Resume picks the correct active status based on media type.
            if (ImGui::Button("Resume", ImVec2(90, 0))) {
                currentLibrary[i].status = (currentLibrary[i].type == "Anime") ? "Watching" : "Reading";
                logActivity("Status Changed: [" + currentLibrary[i].title + "] resumed from Dropped.");
                saveLibrary();
            }
            ImGui::SameLine();

            if (ImGui::Button("Edit")) {
                editingIndex = i;
                snprintf(inputTitle, sizeof(inputTitle), "%s", currentLibrary[i].title.c_str());
                inputTypeIndex = (currentLibrary[i].type == "Anime") ? 0 : 1;
                inputCurrent = currentLibrary[i].currentProgress;
                inputTotal = currentLibrary[i].totalProgress;
                inputRating = currentLibrary[i].rating;
                inputStatusIndex = 2;
                currentTab = EDIT_MEDIA;
            }
            ImGui::SameLine();

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
            string dDelPopupId = "DelConf_Dropped_" + to_string(i);
            if (ImGui::Button("Delete")) ImGui::OpenPopup(dDelPopupId.c_str());
            ImGui::PopStyleColor(2);

            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            if (ImGui::BeginPopupModal(dDelPopupId.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("Delete \"%s\"?", currentLibrary[i].title.c_str());
                ImGui::Text("This action cannot be undone.");
                ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
                if (ImGui::Button("Yes, Delete", ImVec2(120, 35))) { droppedDelete_target = i; ImGui::CloseCurrentPopup(); }
                ImGui::PopStyleColor(2);
                ImGui::SameLine(); ImGui::Spacing(); ImGui::SameLine();
                if (ImGui::Button("Cancel", ImVec2(120, 35))) ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
            }

            ImGui::PopID();
        }
        ImGui::EndTable();

        if (droppedDelete_target >= 0) {
            logActivity("Record Deleted: [" + currentLibrary[droppedDelete_target].title + "] removed from the library.");
            currentLibrary.erase(currentLibrary.begin() + droppedDelete_target);
            saveLibrary();
        }
    }
}
