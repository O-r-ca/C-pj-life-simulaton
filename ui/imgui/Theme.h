#pragma once
#include <imgui.h>

#include <string>

// 색과 폰트. 화면 모양은 여기서만 바꾼다.
namespace theme {

// 팔레트: 남색 바탕 + 금색 포인트
inline ImU32 rgb(int r, int g, int b, int a = 255) { return IM_COL32(r, g, b, a); }

inline const ImU32 Bg         = IM_COL32(13, 16, 23, 255);
inline const ImU32 Panel      = IM_COL32(17, 21, 30, 255);
inline const ImU32 Card       = IM_COL32(22, 27, 38, 255);
inline const ImU32 CardHover  = IM_COL32(30, 36, 50, 255);
inline const ImU32 Border     = IM_COL32(38, 44, 58, 255);
inline const ImU32 Track      = IM_COL32(34, 40, 54, 255);
inline const ImU32 Text       = IM_COL32(232, 228, 216, 255);
inline const ImU32 TextDim    = IM_COL32(138, 143, 156, 255);
inline const ImU32 TextFaint  = IM_COL32(96, 102, 116, 255);
inline const ImU32 Gold       = IM_COL32(212, 168, 75, 255);
inline const ImU32 GoldHover  = IM_COL32(228, 188, 100, 255);
inline const ImU32 Green      = IM_COL32(127, 185, 138, 255);
inline const ImU32 Blue       = IM_COL32(140, 155, 224, 255);
inline const ImU32 Pink       = IM_COL32(208, 122, 154, 255);
inline const ImU32 Teal       = IM_COL32(95, 181, 176, 255);
inline const ImU32 Coral      = IM_COL32(224, 123, 106, 255);

struct Fonts {
    ImFont* body = nullptr;   // 맑은 고딕 18
    ImFont* small = nullptr;  // 맑은 고딕 15 (라벨)
    ImFont* bold = nullptr;   // 맑은 고딕 굵게 18
    ImFont* title = nullptr;  // 맑은 고딕 굵게 26
    ImFont* serif = nullptr;  // 바탕 20 (이야기 본문)
    ImFont* big = nullptr;    // 맑은 고딕 굵게 46 (나이 숫자 전용)
};
extern Fonts fonts;

// extraText: 기본 한글 2350자 외에 꼭 넣을 글자 (스토리 JSON 내용 등)
void loadFonts(ImGuiIO& io, const std::string& extraText);
void apply();  // ImGui + ImPlot 스타일

}  // namespace theme
