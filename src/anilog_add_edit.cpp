// =============================================================================
//  anilog_add_edit.cpp  -  TAB 2: ADD / EDIT A MEDIA RECORD
// -----------------------------------------------------------------------------
//  WHAT THIS TAB DOES
//    Draws the single form used both to create a brand-new title and to edit an
//    existing one. The same function powers both modes; it simply checks
//    `currentTab` to decide whether it is in ADD mode or EDIT mode and changes
//    its title and save behavior accordingly. The form collects: title, type
//    (Anime/Manga), progress (current / total), a 1-5 star rating, and a status.
//    On save it validates the title, builds or updates a MediaRecord, writes an
//    activity-log entry, persists to disk, and returns to the library tab.
//
//  HOW THE LAYOUT WORKS (and where to change spacing)
//    Every field is laid out as a left-aligned column. The horizontal start of
//    that column is controlled by ONE value, `offsetX`. It is initialized from
//    ImGui::GetCursorPosX() at the top of the function, which is the window's
//    natural left padding - the exact same margin the Library, Statistics,
//    Activity Log, and Help tabs begin at. Seeding it this way is what keeps
//    this tab aligned with the rest of the app. To indent the form further from
//    the left edge, add to `offsetX` (e.g. `offsetX += 16.0f`). To change the
//    form's width, edit `formW`.
//
//  WHERE TO CHANGE THINGS
//    - Left margin of the whole form ...... `offsetX` (below).
//    - Form / field width ................. `formW`.
//    - Text sizes ......................... FONT_SCALE_* in anilog_globals.h.
//    - What gets saved .................... the MediaRecord build blocks in the
//                                           "Save" button handler.
// =============================================================================

#include "anilog_globals.h"

void RenderAddEditTab(ImVec2 center) {
    bool isEdit = (currentTab == EDIT_MEDIA);

    // -- Left margin for the whole form --
    // `offsetX` is the X position every field, label, and button starts at.
    // We seed it from the cursor's current X, which at this point is the
    // window's own left padding (the same margin the other tabs naturally use).
    // Previously this was hard-coded to 0.0f, which pinned the form to the
    // absolute window edge and made it sit further left than every other tab -
    // reading it from the cursor instead keeps the alignment consistent.
    // Increase `offsetX` to indent the form more; change `formW` to resize it.
    float formW   = 520.0f;
    float offsetX = ImGui::GetCursorPosX();

    // -- Section title --
    ImGui::SetWindowFontScale(FONT_SCALE_HEADER);
    ImGui::TextColored(COLOR_ACCENT_BLUE, isEdit ? "Edit Record" : "Add New Record");
    ImGui::SetWindowFontScale(FONT_SCALE_BODY);
    ImGui::Spacing();
    ImGui::SetCursorPosX(offsetX);
    ImGui::PushItemWidth(formW);
    ImGui::Separator();
    ImGui::PopItemWidth();
    ImGui::Spacing(); ImGui::Spacing();

    // -- Validation --
    string validationMsg = "";
    string titleStr  = trimStr(string(inputTitle));
    bool   titleBlank= titleStr.empty();
    bool   titleDupe = !titleBlank && titleExists(titleStr, editingIndex);
    if (titleBlank && string(inputTitle) != "")
        validationMsg = "Title cannot be only spaces.";
    else if (titleDupe)
        validationMsg = "A record with this title already exists.";

    // -- Title field --
    ImGui::SetCursorPosX(offsetX);
    ImGui::TextColored(COLOR_LABEL, "Title");
    ImGui::SetCursorPosX(offsetX);
    ImGui::SetNextItemWidth(formW);
    ImGui::InputText("##title", inputTitle, IM_ARRAYSIZE(inputTitle));
    if (!validationMsg.empty()) {
        ImGui::SetCursorPosX(offsetX);
        ImGui::TextColored(COLOR_ACCENT_RED, "%s", validationMsg.c_str());
    }
    ImGui::Spacing(); ImGui::Spacing();

    // -- Category Type --
    ImGui::SetCursorPosX(offsetX);
    ImGui::TextColored(COLOR_LABEL, "Category Type");
    ImGui::SetCursorPosX(offsetX);
    ImGui::RadioButton("Anime", &inputTypeIndex, 0);
    ImGui::SameLine();
    ImGui::RadioButton("Manga", &inputTypeIndex, 1);
    ImGui::Spacing(); ImGui::Spacing();

    // -- Progress fields side by side --
    float halfW = (formW - 16.0f) * 0.5f;
    ImGui::SetCursorPosX(offsetX);
    ImGui::TextColored(COLOR_LABEL,
        inputTypeIndex == 0 ? "Episodes Watched" : "Chapters Read");
    ImGui::SameLine(offsetX + halfW + 16.0f);
    ImGui::TextColored(COLOR_LABEL,
        inputTypeIndex == 0 ? "Total Episodes" : "Total Chapters");

    ImGui::SetCursorPosX(offsetX);
    ImGui::SetNextItemWidth(halfW);
    ImGui::InputInt("##current", &inputCurrent);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(halfW);
    ImGui::InputInt("##total", &inputTotal);

    if (inputCurrent < 0)          inputCurrent = 0;
    if (inputTotal   < 1)          inputTotal   = 1;
    if (inputCurrent > inputTotal) inputCurrent = inputTotal;

    if (inputCurrent == inputTotal && inputTotal > 0) {
        ImGui::SetCursorPosX(offsetX);
        ImGui::TextColored(COLOR_ACCENT_GREEN,
            "Progress matches total - will be marked Completed.");
    }
    ImGui::Spacing(); ImGui::Spacing();

    // -- Star Rating --
    ImGui::SetCursorPosX(offsetX);
    ImGui::TextColored(COLOR_LABEL, "Rating");
    ImGui::SetCursorPosX(offsetX);
    for (int s = 1; s <= 5; s++) {
        if (s > 1) ImGui::SameLine(0, 6);
        bool filled = (s <= inputRating);
        ImGui::PushStyleColor(ImGuiCol_Button,
            filled ? ImVec4(1.0f, 0.8f, 0.1f, 1.0f) : ImVec4(0.25f, 0.25f, 0.30f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.85f, 0.3f, 1.0f));
        char starId[8];
        snprintf(starId, sizeof(starId), "##s%d", s);
        if (ImGui::Button(filled ? ("[*]" + string(starId)).c_str()
                                 : ("[ ]" + string(starId)).c_str(), ImVec2(48, 36)))
            inputRating = s;
        ImGui::PopStyleColor(2);
    }
    ImGui::SameLine(0, 12);
    ImGui::SetWindowFontScale(FONT_SCALE_SMALL);
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.1f, 1.0f), "%d / 5", inputRating);
    ImGui::SetWindowFontScale(FONT_SCALE_BODY);
    ImGui::Spacing(); ImGui::Spacing();

    // -- Status --
    ImGui::SetCursorPosX(offsetX);
    ImGui::TextColored(COLOR_LABEL, "Status");
    ImGui::SetCursorPosX(offsetX);
    const char** currentStatusOptions = (inputTypeIndex == 0) ? animeStatusOptions : mangaStatusOptions;
    ImGui::SetNextItemWidth(formW);
    ImGui::Combo("##status", &inputStatusIndex, currentStatusOptions, 3);
    ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();

    // -- Save / Cancel buttons --
    bool  canSave = !titleBlank && !titleDupe;
    float btnW    = (formW - 12.0f) * 0.5f;

    ImGui::SetCursorPosX(offsetX);
    if (!canSave) {
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
    }

    if (ImGui::Button(isEdit ? "Save Changes" : "Save Record", ImVec2(btnW, 48)) && canSave) {
        if (currentTab == ADD_MEDIA) {
            MediaRecord newMedia;
            newMedia.title           = titleStr;
            newMedia.type            = typeOptions[inputTypeIndex];
            newMedia.currentProgress = inputCurrent;
            newMedia.totalProgress   = inputTotal;
            newMedia.rating          = inputRating;
            newMedia.status          = currentStatusOptions[inputStatusIndex];
            newMedia.dateStarted     = getCurrentDate();
            newMedia.rereadCount     = 0;
            if (inputCurrent == inputTotal) {
                newMedia.status       = "Completed";
                newMedia.dateFinished = getCurrentDate();
            } else {
                newMedia.dateFinished = "";
            }
            currentLibrary.push_back(newMedia);
            logActivity("Record Added: [" + newMedia.title + "] added. Type: "
                      + newMedia.type + ", Status: " + newMedia.status
                      + ", Rating: " + to_string(newMedia.rating));
        } else {
            if (editingIndex >= 0 && editingIndex < (int)currentLibrary.size()) {
                MediaRecord before = currentLibrary[editingIndex];
                currentLibrary[editingIndex].title           = titleStr;
                currentLibrary[editingIndex].type            = typeOptions[inputTypeIndex];
                currentLibrary[editingIndex].currentProgress = inputCurrent;
                currentLibrary[editingIndex].totalProgress   = inputTotal;
                currentLibrary[editingIndex].rating          = inputRating;
                currentLibrary[editingIndex].status          = currentStatusOptions[inputStatusIndex];
                if (inputCurrent == inputTotal) {
                    currentLibrary[editingIndex].status = "Completed";
                    if (currentLibrary[editingIndex].dateFinished.empty())
                        currentLibrary[editingIndex].dateFinished = getCurrentDate();
                } else {
                    currentLibrary[editingIndex].dateFinished = "";
                }
                string changes = buildChangeSummary(before, currentLibrary[editingIndex]);
                logActivity("Record Updated: [" + titleStr + "] - " + changes);
            }
        }
        saveLibrary();
        currentTab = LIBRARY;
        resetForm();
    }

    if (!canSave) ImGui::PopStyleColor(3);

    ImGui::SameLine(0, 12);
    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.35f, 0.20f, 0.20f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.55f, 0.25f, 0.25f, 1.0f));
    if (ImGui::Button("Cancel", ImVec2(btnW, 48))) {
        currentTab = LIBRARY;
        resetForm();
    }
    ImGui::PopStyleColor(2);
}