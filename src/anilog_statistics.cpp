// =============================================================================
//  anilog_statistics.cpp
//  TAB 3: Statistics — vault totals, progress metrics, insights, bar chart.
// =============================================================================

#include "anilog_globals.h"

void RenderStatisticsTab() {
    ImGui::SetWindowFontScale(1.5f);
    ImGui::Text("Statistics");
    ImGui::Separator(); ImGui::Spacing();

    // ── Aggregate all stats in a single pass ──
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

    // ── SECTION 1: Overall Vault Status ──
    ImGui::SetWindowFontScale(1.4f);
    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.6f, 1.0f), "Overall Vault Status");
    ImGui::Spacing();

    if (ImGui::BeginTable("StatsVault", 2, statFlags)) {
        ImGui::TableSetupColumn("Statistic", ImGuiTableColumnFlags_WidthStretch, 2.0f);
        ImGui::TableSetupColumn("Value",     ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableHeadersRow();

        auto statRow = [](const char* label, const char* value) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::SetWindowFontScale(1.3f); ImGui::Text("%s", label);
            ImGui::TableSetColumnIndex(1); ImGui::SetWindowFontScale(1.3f); ImGui::Text("%s", value);
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

    // ── SECTION 2: Progress Metrics ──
    ImGui::SetWindowFontScale(1.4f);
    ImGui::TextColored(ImVec4(0.35f, 0.65f, 1.0f, 1.0f), "Progress Metrics");
    ImGui::Spacing();

    if (ImGui::BeginTable("StatsProgress", 2, statFlags)) {
        ImGui::TableSetupColumn("Metric", ImGuiTableColumnFlags_WidthStretch, 2.0f);
        ImGui::TableSetupColumn("Value",  ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableHeadersRow();

        auto statRow = [](const char* label, const char* value) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::SetWindowFontScale(1.3f); ImGui::Text("%s", label);
            ImGui::TableSetColumnIndex(1); ImGui::SetWindowFontScale(1.3f); ImGui::Text("%s", value);
        };

        char buf[64];
        snprintf(buf, sizeof(buf), "%d", epsWatched);         statRow("Total Episodes Watched",    buf);
        snprintf(buf, sizeof(buf), "%d", chsRead);            statRow("Total Chapters Read",       buf);
        snprintf(buf, sizeof(buf), "%d", totalRereads);       statRow("Total Rewatches / Rereads", buf);
        snprintf(buf, sizeof(buf), "%.2f / 5.00", avgRating); statRow("Average Rating",            buf);
        ImGui::EndTable();
    }

    ImGui::Spacing(); ImGui::Spacing();

    // ── SECTION 3: Insights ──
    ImGui::SetWindowFontScale(1.4f);
    ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.3f, 1.0f), "Insights");
    ImGui::Spacing();

    if (ImGui::BeginTable("StatsInsights", 2, statFlags)) {
        ImGui::TableSetupColumn("Insight", ImGuiTableColumnFlags_WidthStretch, 2.0f);
        ImGui::TableSetupColumn("Value",   ImGuiTableColumnFlags_WidthStretch, 2.5f);
        ImGui::TableHeadersRow();

        auto insightRow = [](const char* label, const char* value) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::SetWindowFontScale(1.3f); ImGui::Text("%s", label);
            ImGui::TableSetColumnIndex(1); ImGui::SetWindowFontScale(1.3f);
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.6f, 1.0f), "%s", value);
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

    // ── SECTION 4: Progress Overview Bar Chart ──
    ImGui::Spacing(); ImGui::Spacing();
    ImGui::SetWindowFontScale(1.4f);
    ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.3f, 1.0f), "Progress Overview");
    ImGui::Spacing();

    struct BarData { const char* label; float value; float maxVal; ImVec4 color; };
    BarData bars[] = {
        { "Anime",     (float)totalAnime,   10.0f,  ImVec4(0.35f, 0.65f, 1.0f, 1.0f) },
        { "Manga",     (float)totalManga,   10.0f,  ImVec4(0.4f,  1.0f,  0.6f, 1.0f) },
        { "Completed", (float)completedCnt, 10.0f,  ImVec4(0.4f,  1.0f,  0.6f, 1.0f) },
        { "Dropped",   (float)droppedCnt,   10.0f,  ImVec4(1.0f,  0.4f,  0.4f, 1.0f) },
        { "Watching",  (float)watchingCnt,  10.0f,  ImVec4(1.0f,  0.75f, 0.3f, 1.0f) },
        { "Eps",       (float)epsWatched,   100.0f, ImVec4(0.35f, 0.65f, 1.0f, 1.0f) },
        { "Chapters",  (float)chsRead,      100.0f, ImVec4(0.9f,  0.5f,  1.0f, 1.0f) },
    };
    int barCount = IM_ARRAYSIZE(bars);

    ImVec2 canvasPos  = ImGui::GetCursorScreenPos();
    float  canvasW    = ImGui::GetContentRegionAvail().x;
    float  canvasH    = 180.0f;
    float  axisW      = 40.0f;
    float  barPadding = 12.0f;
    float  graphW     = canvasW - axisW;
    float  barW       = (graphW - barPadding * (barCount + 1)) / barCount;
    float  labelH     = 20.0f;
    float  graphH     = canvasH - labelH;

    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Background
    dl->AddRectFilled(canvasPos,
                      ImVec2(canvasPos.x + canvasW, canvasPos.y + canvasH),
                      IM_COL32(30, 30, 40, 255), 6.0f);

    // Y-axis gridlines and labels
    int ySteps = 5;
    for (int s = 0; s <= ySteps; s++) {
        float ratio = (float)s / ySteps;
        float y     = canvasPos.y + graphH * (1.0f - ratio);
        int   pct   = (int)(ratio * 100);

        dl->AddLine(ImVec2(canvasPos.x + axisW, y),
                    ImVec2(canvasPos.x + canvasW, y),
                    IM_COL32(80, 80, 100, 120), 1.0f);

        char lblBuf[8];
        snprintf(lblBuf, sizeof(lblBuf), "%d%%", pct);
        ImVec2 lblSize = ImGui::CalcTextSize(lblBuf);
        dl->AddText(ImVec2(canvasPos.x + axisW - lblSize.x - 4.0f, y - lblSize.y * 0.5f),
                    IM_COL32(180, 180, 180, 255), lblBuf);
    }

    // Vertical axis line
    dl->AddLine(ImVec2(canvasPos.x + axisW, canvasPos.y),
                ImVec2(canvasPos.x + axisW, canvasPos.y + graphH),
                IM_COL32(120, 120, 150, 255), 1.5f);

    // Bars
    for (int i = 0; i < barCount; i++) {
        float ratio = (bars[i].maxVal > 0) ? (bars[i].value / bars[i].maxVal) : 0.0f;
        if (ratio > 1.0f) ratio = 1.0f;

        float x0 = canvasPos.x + axisW + barPadding + i * (barW + barPadding);
        float y0 = canvasPos.y + graphH * (1.0f - ratio);
        float y1 = canvasPos.y + graphH;

        dl->AddRectFilled(ImVec2(x0, y0), ImVec2(x0 + barW, y1),
                          ImGui::ColorConvertFloat4ToU32(bars[i].color), 4.0f);

        char valBuf[16];
        snprintf(valBuf, sizeof(valBuf), "%d", (int)bars[i].value);
        ImVec2 valSize = ImGui::CalcTextSize(valBuf);
        dl->AddText(ImVec2(x0 + (barW - valSize.x) * 0.5f, y0 - valSize.y - 2.0f),
                    IM_COL32(255, 255, 255, 200), valBuf);

        ImVec2 lblSize = ImGui::CalcTextSize(bars[i].label);
        dl->AddText(ImVec2(x0 + (barW - lblSize.x) * 0.5f, canvasPos.y + graphH + 4.0f),
                    IM_COL32(200, 200, 200, 255), bars[i].label);
    }

    ImGui::Dummy(ImVec2(canvasW, canvasH));
}
