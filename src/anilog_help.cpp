// =============================================================================
//  anilog_help.cpp
//  TAB 5: Help — getting started, common problems, tips, about.
// =============================================================================

#include "anilog_globals.h"

void RenderHelpTab() {
    ImGui::SetWindowFontScale(1.5f);
    ImGui::TextColored(ImVec4(0.35f, 0.65f, 1.0f, 1.0f), "Help Guide");
    ImGui::Separator(); ImGui::Spacing();

    ImGui::BeginChild("HelpScroll", ImVec2(0, 0), false);
    ImGui::SetWindowFontScale(1.3f);

    // ── Getting Started ──
    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.6f, 1.0f), "Getting Started");
    ImGui::Separator(); ImGui::Spacing();
    ImGui::TextWrapped("1. Use 'Add Record' in the sidebar to log a new anime or manga.");
    ImGui::TextWrapped("2. Fill in the title, type, progress, rating, and status, then click Save Record.");
    ImGui::TextWrapped("3. Your entry will appear in AniDex under Active, Completed, or Dropped.");
    ImGui::Spacing(); ImGui::Spacing();

    // ── Common Problems ──
    ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.3f, 1.0f), "Common Problems");
    ImGui::Separator(); ImGui::Spacing();

    ImGui::TextColored(ImVec4(0.9f, 0.6f, 0.6f, 1.0f), "Save Record button is grayed out");
    ImGui::TextWrapped("    The title field is empty, contains only spaces, or a record with the same title already exists. Fix the title and try again.");
    ImGui::Spacing();

    ImGui::TextColored(ImVec4(0.9f, 0.6f, 0.6f, 1.0f), "My entry moved to Completed automatically");
    ImGui::TextWrapped("    When current progress equals total episodes/chapters the record auto-completes. Lower the total or adjust progress to keep it Active.");
    ImGui::Spacing();

    ImGui::TextColored(ImVec4(0.9f, 0.6f, 0.6f, 1.0f), "I accidentally deleted a record");
    ImGui::TextWrapped("    Deletions are permanent and cannot be undone. Re-add the record manually via Add Record.");
    ImGui::Spacing();

    ImGui::TextColored(ImVec4(0.9f, 0.6f, 0.6f, 1.0f), "Unsaved Changes popup appeared");
    ImGui::TextWrapped("    You navigated away while the Add/Edit form had content. Choose 'Keep Editing' to return or 'Yes, Discard' to leave without saving.");
    ImGui::Spacing();

    ImGui::TextColored(ImVec4(0.9f, 0.6f, 0.6f, 1.0f), "Activity Log shows no entries");
    ImGui::TextWrapped("    Logs generate automatically when you add, edit, delete, or update progress on a record.");
    ImGui::Spacing();

    ImGui::TextColored(ImVec4(0.9f, 0.6f, 0.6f, 1.0f), "Statistics show 0 for everything");
    ImGui::TextWrapped("    Stats are calculated live from your library. Add some records first and they will populate.");
    ImGui::Spacing(); ImGui::Spacing();

    // ── Tips ──
    ImGui::TextColored(ImVec4(0.9f, 0.5f, 1.0f, 1.0f), "Tips & Tricks");
    ImGui::Separator(); ImGui::Spacing();
    ImGui::TextWrapped("- Use the Search bar in AniDex to quickly find a title by name.");
    ImGui::Spacing();
    ImGui::TextWrapped("- Sort buttons (A-Z, Z-A, Rating, Progress) reorder your entire library instantly.");
    ImGui::Spacing();
    ImGui::TextWrapped("- '+1 Episode / +1 Chapter' in AniDex is the fastest way to log progress without opening the Edit form.");
    ImGui::Spacing();
    ImGui::TextWrapped("- Completed titles can be Rewatched or Reread — this resets progress to 0 and moves them back to Active.");
    ImGui::Spacing();
    ImGui::TextWrapped("- Dropped titles can be Resumed at any time from the Dropped section in AniDex.");
    ImGui::Spacing();
    ImGui::TextWrapped("- The Statistics tab updates in real time — check it after adding records to see your progress overview and bar chart.");
    ImGui::Spacing(); ImGui::Spacing();

    // ── About ──
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "About");
    ImGui::Separator(); ImGui::Spacing();
    ImGui::TextWrapped("ANILOG - Anime & Manga Media Tracker");
    ImGui::TextWrapped("COMP 003 | Final Laboratory Project");
    ImGui::TextWrapped("Built with Dear ImGui, GLFW, and OpenGL 3.");

    ImGui::EndChild();
}
