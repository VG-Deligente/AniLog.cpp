// =============================================================================
//  anilog_statistics.cpp  -  TAB 3: STATISTICS
// -----------------------------------------------------------------------------
//  WHAT THIS TAB DOES
//    Reads the whole library once, tallies it up, and presents the numbers as
//    three stacked tables, top to bottom:
//      1. Overall Vault Status .. totals per type/status + completion rate.
//      2. Progress Metrics ...... episodes watched, chapters read, avg rating.
//      3. Insights .............. "most watched", "highest rated", etc.
//    Everything is recomputed every frame from `currentLibrary`, so the page is
//    always live - add or edit a record and the numbers update immediately.
//
//  HOW THE PAGE IS BUILT
//    The whole tab is one function, RenderStatisticsTab(). It first walks the
//    library a single time to accumulate every counter it needs (totals, status
//    counts, episodes, chapters, ratings, "most/highest" picks). After that it
//    just lays out the three tables using those accumulated values. There is no
//    chart or drawing code - the tab is purely tabular.
//
//  WHERE TO CHANGE THINGS
//    - Add / rename a row ...... edit the statRow / insightRow calls in the
//                                matching table block below.
//    - Add a new metric ........ accumulate it in the single pass at the top of
//                                RenderStatisticsTab(), then print it in a row.
//    - Section colors .......... the COLOR_* passed to each section heading.
//    - Text size ............... FONT_SCALE_* in anilog_globals.h.
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
    string mostWatchedTitle = "N/A", mostReadTitle = "N/A";
    int   mostWatchedEps = -1, mostReadChs = -1;
    string highestRatedTitle = "N/A";
    int   highestRating = 0;

    for (int i = 0; i < (int)currentLibrary.size(); i++) {
        const MediaRecord& m = currentLibrary[i];
        if (m.type == "Anime") { totalAnime++; epsWatched += m.currentProgress; }
        else                   { totalManga++; chsRead    += m.currentProgress; }
        if (m.status == "Completed") completedCnt++;
        if (m.status == "Dropped")   droppedCnt++;
        if (m.status == "Watching")  watchingCnt++;
        if (m.status == "Reading")   readingCnt++;
        totalRereads += m.rereadCount;
        sumRating += m.rating;
        ratedCount++;
        if (m.type == "Anime" && m.currentProgress > mostWatchedEps) {
            mostWatchedEps   = m.currentProgress;
            mostWatchedTitle = m.title;
        }
        if (m.type == "Manga" && m.currentProgress > mostReadChs) {
            mostReadChs   = m.currentProgress;
            mostReadTitle = m.title;
        }
        if (m.rating > highestRating) {
            highestRating     = m.rating;
            highestRatedTitle = m.title;
        }
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

    // -- SECTION 3: Insights --
    ImGui::SetWindowFontScale(FONT_SCALE_SECTION);
    ImGui::TextColored(COLOR_ACCENT_ORANGE, "Insights");
    ImGui::SetWindowFontScale(FONT_SCALE_BODY);
    ImGui::Spacing();

    if (ImGui::BeginTable("StatsInsights", 2, statFlags)) {
        ImGui::TableSetupColumn("Insight", ImGuiTableColumnFlags_WidthStretch, 2.0f);
        ImGui::TableSetupColumn("Value",   ImGuiTableColumnFlags_WidthStretch, 2.5f);
        ImGui::TableHeadersRow();

        auto insightRow = [](const char* label, const char* value) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::SetWindowFontScale(FONT_SCALE_BODY); ImGui::Text("%s", label);
            ImGui::TableSetColumnIndex(1); ImGui::SetWindowFontScale(FONT_SCALE_BODY);
            ImGui::TextColored(COLOR_ACCENT_GREEN, "%s", value);
        };

        char buf[128];
        snprintf(buf, sizeof(buf), "%.1f%%", completionRate);
        insightRow("Completion Rate", buf);
        insightRow("Most Watched Anime", mostWatchedTitle.c_str());
        snprintf(buf, sizeof(buf), "%d episodes", mostWatchedEps < 0 ? 0 : mostWatchedEps);
        insightRow("  Episodes Watched", buf);
        insightRow("Most Read Manga", mostReadTitle.c_str());
        snprintf(buf, sizeof(buf), "%d chapters", mostReadChs < 0 ? 0 : mostReadChs);
        insightRow("  Chapters Read", buf);
        insightRow("Highest Rated Title", highestRatedTitle.c_str());
        snprintf(buf, sizeof(buf), "%d / 5", highestRating > 0 ? highestRating : 0);
        insightRow("  Rating", buf);
        ImGui::EndTable();
    }
}