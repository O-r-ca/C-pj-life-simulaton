#include "Theme.h"

#include <implot.h>

#include <cstdio>

namespace theme {

Fonts fonts;

namespace {

const char* kCommonHangul =
#include "KoreanCommon.inc"
    ;

bool fileExists(const char* path) {
    if (FILE* f = std::fopen(path, "rb")) {
        std::fclose(f);
        return true;
    }
    return false;
}

ImVec4 v4(ImU32 c) { return ImGui::ColorConvertU32ToFloat4(c); }

}  // namespace

void loadFonts(ImGuiIO& io, const std::string& extraText) {
    // 한글 전체(11,172자)를 크기별로 다 넣으면 텍스처가 너무 커서,
    // 자주 쓰는 2350자 + 스토리에 실제로 나오는 글자만 넣는다.
    static ImVector<ImWchar> textRanges;
    static ImVector<ImWchar> digitRanges;
    {
        ImFontGlyphRangesBuilder b;
        b.AddRanges(io.Fonts->GetGlyphRangesDefault());
        b.AddText(kCommonHangul);
        b.AddText(extraText.c_str());
        b.AddText(u8"·…›‹—–“”‘’▸▾●○");
        b.BuildRanges(&textRanges);
    }
    {
        ImFontGlyphRangesBuilder b;
        b.AddText(u8"0123456789-세");
        b.BuildRanges(&digitRanges);
    }

    const char* regular = "C:/Windows/Fonts/malgun.ttf";
    const char* boldPath = "C:/Windows/Fonts/malgunbd.ttf";
    const char* serifPath = "C:/Windows/Fonts/batang.ttc";

    if (!fileExists(regular)) {
        ImFont* f = io.Fonts->AddFontDefault();
        fonts = {f, f, f, f, f, f};
        return;
    }
    if (!fileExists(boldPath)) boldPath = regular;
    if (!fileExists(serifPath)) serifPath = regular;

    fonts.body = io.Fonts->AddFontFromFileTTF(regular, 18.0f, nullptr, textRanges.Data);  // 첫 폰트 = 기본
    fonts.small = io.Fonts->AddFontFromFileTTF(regular, 15.0f, nullptr, textRanges.Data);
    fonts.bold = io.Fonts->AddFontFromFileTTF(boldPath, 18.0f, nullptr, textRanges.Data);
    fonts.title = io.Fonts->AddFontFromFileTTF(boldPath, 26.0f, nullptr, textRanges.Data);
    fonts.serif = io.Fonts->AddFontFromFileTTF(serifPath, 20.0f, nullptr, textRanges.Data);
    fonts.big = io.Fonts->AddFontFromFileTTF(boldPath, 46.0f, nullptr, digitRanges.Data);
}

void apply() {
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowPadding = ImVec2(0, 0);
    s.WindowBorderSize = 0;
    s.ChildBorderSize = 1;
    s.FrameBorderSize = 1;
    s.WindowRounding = 0;
    s.ChildRounding = 10;
    s.FrameRounding = 7;
    s.GrabRounding = 4;
    s.ScrollbarRounding = 6;
    s.ScrollbarSize = 10;
    s.FramePadding = ImVec2(14, 10);
    s.ItemSpacing = ImVec2(10, 10);
    s.ItemInnerSpacing = ImVec2(8, 6);

    ImVec4* c = s.Colors;
    c[ImGuiCol_WindowBg] = v4(Bg);
    c[ImGuiCol_ChildBg] = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_PopupBg] = v4(Card);
    c[ImGuiCol_Border] = v4(Border);
    c[ImGuiCol_Text] = v4(Text);
    c[ImGuiCol_TextDisabled] = v4(TextDim);
    c[ImGuiCol_FrameBg] = v4(rgb(10, 13, 19));
    c[ImGuiCol_FrameBgHovered] = v4(rgb(14, 18, 26));
    c[ImGuiCol_FrameBgActive] = v4(rgb(14, 18, 26));
    c[ImGuiCol_Button] = v4(Card);
    c[ImGuiCol_ButtonHovered] = v4(CardHover);
    c[ImGuiCol_ButtonActive] = v4(CardHover);
    c[ImGuiCol_CheckMark] = v4(Gold);
    c[ImGuiCol_SliderGrab] = v4(Gold);
    c[ImGuiCol_SliderGrabActive] = v4(GoldHover);
    c[ImGuiCol_Header] = v4(Card);
    c[ImGuiCol_HeaderHovered] = v4(CardHover);
    c[ImGuiCol_HeaderActive] = v4(CardHover);
    c[ImGuiCol_ScrollbarBg] = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_ScrollbarGrab] = v4(rgb(42, 49, 66));
    c[ImGuiCol_ScrollbarGrabHovered] = v4(rgb(56, 64, 84));
    c[ImGuiCol_ScrollbarGrabActive] = v4(rgb(56, 64, 84));
    c[ImGuiCol_Separator] = v4(Border);
    c[ImGuiCol_TextSelectedBg] = v4(rgb(212, 168, 75, 90));
    c[ImGuiCol_NavHighlight] = v4(Gold);

    ImPlotStyle& p = ImPlot::GetStyle();
    p.PlotPadding = ImVec2(12, 12);
    p.LineWeight = 2.5f;
    p.Colors[ImPlotCol_FrameBg] = ImVec4(0, 0, 0, 0);
    p.Colors[ImPlotCol_PlotBg] = ImVec4(0, 0, 0, 0);
    p.Colors[ImPlotCol_PlotBorder] = v4(Border);
    p.Colors[ImPlotCol_LegendBg] = v4(Card);
    p.Colors[ImPlotCol_LegendBorder] = v4(Border);
    p.Colors[ImPlotCol_LegendText] = v4(Text);
    p.Colors[ImPlotCol_AxisText] = v4(TextDim);
    p.Colors[ImPlotCol_AxisGrid] = v4(rgb(38, 44, 58, 160));
    p.Colors[ImPlotCol_AxisBg] = ImVec4(0, 0, 0, 0);
    p.Colors[ImPlotCol_AxisBgHovered] = v4(rgb(38, 44, 58, 80));
    p.Colors[ImPlotCol_AxisBgActive] = v4(rgb(38, 44, 58, 120));
}

}  // namespace theme
