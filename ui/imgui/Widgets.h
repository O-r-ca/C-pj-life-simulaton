#pragma once
#include <imgui.h>

#include <string>

// ImGui 기본 위젯 위에 그린 커스텀 위젯들 (카드, 막대, 나이 표시 등)
namespace ui {

// 둥근 테두리 카드. 높이는 내용에 맞게 자동 (width 0 = 가로 꽉 채움)
void beginCard(const char* id, float width = 0);
void endCard();

// 고정 크기 패널 (스크롤 가능)
void beginPanel(const char* id, ImVec2 size, ImU32 bg);
void endPanel();

// 작은 회색 라벨
void label(const char* text);

// "라벨  ━━━━━──  값" 형태의 막대
void statBar(const char* name, const std::string& value, float fraction, ImU32 color);

// "키   값" 한 줄
void keyValue(const char* key, const std::string& value);

// ── 29세 ── 가운데 알약 모양 나이 표시
void agePill(const std::string& text, bool active);

// 왼쪽에 색 막대가 있는 본문 단락 (wrapWidth 안에서 줄바꿈)
void narrative(const std::string& text, ImFont* font, ImU32 textColor, ImU32 barColor, float wrapWidth);

// 선택지 버튼 (카드 모양, 마우스를 올리면 금색 테두리)
bool choiceButton(const char* id, const std::string& text, float width);

// 금색으로 채운 주요 버튼
bool goldButton(const char* text, ImVec2 size = ImVec2(0, 0));

// 테두리만 있는 보조 버튼
bool ghostButton(const char* text);

// [A | B | C] 붙어 있는 토글 버튼. 바뀌면 true
bool segmented(const char* id, const char* const* labels, int count, int* current);
float segmentedWidth(const char* const* labels, int count);

// 얇은 진행 막대
void progressBar(float fraction, float width, float height, ImU32 color);

// 천 단위 쉼표 (예: -12,345)
std::string formatNumber(double v);

}  // namespace ui
