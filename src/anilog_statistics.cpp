// =============================================================================
//  anilog_statistics.cpp  -  TAB 3: STATISTICS
// -----------------------------------------------------------------------------
//  WHAT THIS TAB DOES
//    Reads the whole library once, tallies it up, and presents the numbers as
//    two stacked tables, top to bottom:
//      1. Overall Vault Status .. totals per type/status + completion rate.
//      2. Progress Metrics ...... episodes watched, chapters read, avg rating.
//    Everything is recomputed every frame from `currentLibrary`, so the page is
//    always live - add or edit a record and the numbers update immediately.
// =============================================================================

#include "anilog_globals.h"

void RenderStatisticsTab() {
    ImGui::SetWindowFontScale(FONT_SCALE_HEADER);
    ImGui::TextColored(COLOR_ACCENT_BLUE, "Statistics");
    ImGui::SetWindowFontScale(FONT_SCALE_BODY);
    ImGui::Separator(); ImGui::Spacing();

    // -- Aggregate all stats in a single pass --
    int totalAnime = 0, totalManga = 0;
    int epsWatched = 0, chsRead = 0;
    int completedCnt = 0, droppedCnt = 0, watchingCnt = 0, readingCnt = 0;
    int totalRereads = 0;
    float sumRating = 0.0f;
    int   ratedCount = 0;
    for (int i = 0; i < (int)currentLibrary.size(); i++) {
        const MediaRecord& m = currentLibrary[i];
        
        // Calculate true progress including rewatches/rereads
        int totalMediaProgress = m.currentProgress + (m.rereadCount * m.totalProgress);

        if (m.type == "Anime") { 
            totalAnime++; 
            epsWatched += totalMediaProgress; 
        } else { 
            totalManga++; 
            chsRead += totalMediaProgress; 
        }

        if (m.status == "Completed") completedCnt++;
        if (m.status == "Dropped")   droppedCnt++;
        if (m.status == "Watching")  watchingCnt++;
        if (m.status == "Reading")   readingCnt++;
        
        totalRereads += m.rereadCount;
        sumRating += m.rating;
        ratedCount++;
        
    }

    float avgRating      = (ratedCount > 0) ? sumRating / ratedCount : 0.0f;
    float completionRate = (currentLibrary.size() > 0)
        ? (completedCnt * 100.0f / currentLibrary.size()) : 0.0f;

    ImGuiTableFlags statFlags = ImGuiTableFlags_Borders
                              | ImGuiTableFlags_RowBg
                              | ImGuiTableFlags_SizingStretchProp;

    // -- SECTION 1: Overall Vault Status --
    ImGui::SetWindowFontScale(FONT_SCALE_SECTION);
    ImGui::TextColored(COLOR_ACCENT_GREEN, "Overall Vault Status");
    ImGui::SetWindowFontScale(FONT_SCALE_BODY);
    ImGui::Spacing();

    if (ImGui::BeginTable("StatsVault", 2, statFlags)) {
        ImGui::TableSetupColumn("Statistic", ImGuiTableColumnFlags_WidthStretch, 2.0f);
        ImGui::TableSetupColumn("Value",     ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableHeadersRow();

        auto statRow = [](const char* label, const char* value) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::SetWindowFontScale(FONT_SCALE_BODY); ImGui::Text("%s", label);
            ImGui::TableSetColumnIndex(1); ImGui::SetWindowFontScale(FONT_SCALE_BODY); ImGui::Text("%s", value);
        };

        char buf[64];
        snprintf(buf, sizeof(buf), "%d", totalAnime);         statRow("Total Anime Tracked",  buf);
        snprintf(buf, sizeof(buf), "%d", totalManga);         statRow("Total Manga Tracked",  buf);
        snprintf(buf, sizeof(buf), "%d", completedCnt);       statRow("Completed Titles",     buf);
        snprintf(buf, sizeof(buf), "%d", droppedCnt);         statRow("Dropped Titles",       buf);
        snprintf(buf, sizeof(buf), "%d", watchingCnt);        statRow("Currently Watching",   buf);
        snprintf(buf, sizeof(buf), "%d", readingCnt);         statRow("Currently Reading",    buf);
        snprintf(buf, sizeof(buf), "%.1f%%", completionRate); statRow("Completion Rate",      buf);
        ImGui::EndTable();
    }

    ImGui::Spacing(); ImGui::Spacing();

    // -- SECTION 2: Progress Metrics --
    ImGui::SetWindowFontScale(FONT_SCALE_SECTION);
    ImGui::TextColored(COLOR_ACCENT_BLUE, "Progress Metrics");
    ImGui::SetWindowFontScale(FONT_SCALE_BODY);
    ImGui::Spacing();

    if (ImGui::BeginTable("StatsProgress", 2, statFlags)) {
        ImGui::TableSetupColumn("Metric", ImGuiTableColumnFlags_WidthStretch, 2.0f);
        ImGui::TableSetupColumn("Value",  ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableHeadersRow();

        auto statRow = [](const char* label, const char* value) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::SetWindowFontScale(FONT_SCALE_BODY); ImGui::Text("%s", label);
            ImGui::TableSetColumnIndex(1); ImGui::SetWindowFontScale(FONT_SCALE_BODY); ImGui::Text("%s", value);
        };

        char buf[64];
        snprintf(buf, sizeof(buf), "%d", epsWatched);         statRow("Total Episodes Watched",    buf);
        snprintf(buf, sizeof(buf), "%d", chsRead);            statRow("Total Chapters Read",       buf);
        snprintf(buf, sizeof(buf), "%d", totalRereads);       statRow("Total Rewatches / Rereads", buf);
        snprintf(buf, sizeof(buf), "%.2f / 5.00", avgRating); statRow("Average Rating",            buf);
        ImGui::EndTable();
    }

    ImGui::Spacing(); ImGui::Spacing();
}
